// ============================================================
// CALWF - LLM 输出导入器实现
// ============================================================

#include "calwf/io/LlmImporter.h"
#include "calwf/io/CSVParser.h"
#include "calwf/text/SentenceSplitter.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/StringUtils.h"
#include "calwf/utils/Logger.h"

#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 元数据键：model_name + "|" + prompt_id + "|" + run
// ------------------------------------------------------------
static std::string metaKey(const std::string& model,
                           const std::string& prompt_id,
                           int run) {
    return model + "|" + prompt_id + "|" + std::to_string(run);
}

// ------------------------------------------------------------
// 加载元数据
// ------------------------------------------------------------
bool LlmImporter::loadMetadata(
    const std::string& meta_path,
    std::unordered_map<std::string, LlmMetadataEntry>& table) {

    if (!utils::fileExists(meta_path)) {
        CALWF_LOG_WARN("LLM 元数据文件不存在：%s", meta_path.c_str());
        return false;
    }

    std::vector<std::string> header;
    auto rows = CSVParser::parseFileWithHeader(meta_path, header);
    if (header.empty()) {
        CALWF_LOG_ERROR("LLM 元数据加载失败：无表头");
        return false;
    }

    // 去 BOM
    if (!header.empty() && !header[0].empty()) {
        std::string& h0 = header[0];
        if (h0.size() >= 3 &&
            static_cast<unsigned char>(h0[0]) == 0xEF &&
            static_cast<unsigned char>(h0[1]) == 0xBB &&
            static_cast<unsigned char>(h0[2]) == 0xBF) {
            h0 = h0.substr(3);
        }
    }

    // 列索引映射
    std::unordered_map<std::string, int> col;
    for (std::size_t i = 0; i < header.size(); ++i) {
        col[header[i]] = static_cast<int>(i);
    }

    auto getCol = [&](const std::vector<std::string>& row,
                      const std::string& name) -> std::string {
        auto it = col.find(name);
        if (it == col.end()) return "";
        int idx = it->second;
        if (idx < 0 || static_cast<std::size_t>(idx) >= row.size()) return "";
        return row[static_cast<std::size_t>(idx)];
    };

    int loaded = 0;
    for (const auto& row : rows) {
        if (row.empty()) continue;

        LlmMetadataEntry e;
        e.doc_id         = getCol(row, "doc_id");
        e.model_name     = getCol(row, "model_name");
        e.model_version  = getCol(row, "model_version");
        e.prompt_id      = getCol(row, "prompt_id");
        e.topic          = getCol(row, "topic");
        e.genre          = getCol(row, "genre");
        e.target_level   = getCol(row, "target_level");
        e.run_number     = utils::toInt(getCol(row, "run_number"), -1);
        e.deep_thinking  = (utils::toLower(getCol(row, "deep_thinking")) == "true");
        e.web_access     = (utils::toLower(getCol(row, "web_access")) == "true");
        e.date_generated = getCol(row, "date_generated");
        e.file_path      = getCol(row, "file_path");
        e.notes          = getCol(row, "notes");

        if (e.model_name.empty() || e.prompt_id.empty() || e.run_number < 0) {
            continue;
        }

        table[metaKey(e.model_name, e.prompt_id, e.run_number)] = std::move(e);
        ++loaded;
    }

    CALWF_LOG_INFO("LLM 元数据加载完成：%d 条", loaded);
    return true;
}

// ------------------------------------------------------------
// 从文件名解析 prompt_id 和 run_number
// 例如 "p001_run1" → ("p001", 1)
// ------------------------------------------------------------
static bool parseFileName(const std::string& stem,
                          std::string& prompt_id,
                          int& run_number) {
    // 找 "_run"
    std::size_t pos = stem.find("_run");
    if (pos == std::string::npos) {
        return false;
    }
    prompt_id = stem.substr(0, pos);
    std::string run_str = stem.substr(pos + 4);
    run_number = utils::toInt(run_str, -1);
    return (run_number >= 0 && !prompt_id.empty());
}

// ------------------------------------------------------------
// 导入单个文件
// ------------------------------------------------------------
bool LlmImporter::loadOneFile(const std::string& path,
                              const LlmMetadataEntry& meta,
                              const std::string& model_name,
                              Document& out) {
    std::string content = utils::readFile(path);
    if (content.empty()) {
        return false;
    }

    // 去掉 BOM
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
    }

    // 填充身份和元数据
    out.doc_id             = meta.doc_id.empty()
                             ? ("LLM_" + model_name + "_" + meta.prompt_id +
                                "_r" + std::to_string(meta.run_number))
                             : meta.doc_id;
    out.source             = SourceType::LLM;
    out.genre              = meta.genre;
    out.model_name         = model_name;
    out.model_version      = meta.model_version;
    out.prompt_id          = meta.prompt_id;
    out.topic              = meta.topic;
    out.task_type          = meta.genre;
    out.target_level       = meta.target_level;
    out.run_number         = meta.run_number;
    out.deep_thinking      = meta.deep_thinking;
    out.web_access         = meta.web_access;
    out.date_generated     = meta.date_generated;

    // 分句、分段
    auto paragraphs = text::SentenceSplitter::splitParagraphs(content);

    int para_id = 0;
    for (auto& sents : paragraphs) {
        Paragraph p;
        p.id = ++para_id;
        std::string ptext;
        int sent_id = 0;
        for (auto& s : sents) {
            Sentence sent;
            sent.id   = ++sent_id;
            sent.text = s;

            // 简单分词
            std::string word;
            int word_id = 0;
            for (char c : s) {
                if (c == ' ' || c == '\t') {
                    if (!word.empty()) {
                        Token t;
                        t.sentence_id = sent.id;
                        t.word_id     = ++word_id;
                        t.word        = word;
                        sent.tokens.push_back(std::move(t));
                        word.clear();
                    }
                } else {
                    word.push_back(c);
                }
            }
            if (!word.empty()) {
                Token t;
                t.sentence_id = sent.id;
                t.word_id     = ++word_id;
                t.word        = word;
                sent.tokens.push_back(std::move(t));
            }

            if (!sent.text.empty()) {
                if (!ptext.empty()) ptext += ' ';
                ptext += sent.text;
                p.sentences.push_back(std::move(sent));
            }
        }
        p.text = ptext;
        if (!p.empty()) {
            out.paragraphs.push_back(std::move(p));
        }
    }

    if (out.paragraphs.empty()) {
        return false;
    }

    std::string full;
    for (std::size_t i = 0; i < out.paragraphs.size(); ++i) {
        if (i > 0) full += "\n\n";
        full += out.paragraphs[i].text;
    }
    out.raw_text = full;
    out.recomputeStats();

    return true;
}

// ------------------------------------------------------------
// 主入口
// ------------------------------------------------------------
std::vector<Document>
LlmImporter::load(const std::string& llm_root_dir,
                  const std::string& meta_path) {
    stats_ = LlmImportStats{};

    // 1. 加载元数据
    std::unordered_map<std::string, LlmMetadataEntry> meta_table;
    loadMetadata(meta_path, meta_table);

    // 2. 遍历 llm_root_dir 下的每个模型目录
    std::vector<Document> result;

    if (!utils::dirExists(llm_root_dir)) {
        CALWF_LOG_WARN("LLM 根目录不存在：%s", llm_root_dir.c_str());
        return result;
    }

    for (const auto& entry : fs::directory_iterator(llm_root_dir)) {
        if (!entry.is_directory()) continue;

        std::string model_name = entry.path().filename().string();
        // 跳过非模型目录
        if (model_name.empty() || model_name[0] == '_' || model_name[0] == '.') {
            continue;
        }

        // 遍历模型目录下的 .txt
        auto txt_files = utils::listFiles(entry.path().string(), "txt");
        stats_.total_files += static_cast<int>(txt_files.size());

        int model_count = 0;

        for (const auto& path : txt_files) {
            std::string stem = utils::fileNameNoExt(path);
            std::string prompt_id;
            int run_number = -1;

            if (!parseFileName(stem, prompt_id, run_number)) {
                ++stats_.skipped_parse;
                continue;
            }

            // 在元数据里查
            auto it = meta_table.find(metaKey(model_name, prompt_id, run_number));
            if (it == meta_table.end()) {
                ++stats_.skipped_no_meta;
                CALWF_LOG_WARN("LLM 无元数据：%s/%s",
                               model_name.c_str(), stem.c_str());
                continue;
            }

            Document doc;
            if (!loadOneFile(path, it->second, model_name, doc)) {
                ++stats_.skipped_empty;
                continue;
            }

            result.push_back(std::move(doc));
            ++stats_.imported;
            ++model_count;
        }

        stats_.per_model[model_name] = model_count;
        CALWF_LOG_INFO("LLM 模型 %s：导入 %d 篇",
                       model_name.c_str(), model_count);
    }

    CALWF_LOG_INFO("LLM 导入完成：成功 %d 篇，"
                   "无元数据 %d，空文档 %d，解析失败 %d",
                   stats_.imported,
                   stats_.skipped_no_meta,
                   stats_.skipped_empty,
                   stats_.skipped_parse);

    return result;
}

} // namespace io
} // namespace calwf