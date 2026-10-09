// ============================================================
// CALWF - BAWE 导入器实现
// ============================================================

#include "calwf/io/BaweImporter.h"
#include "calwf/io/CSVParser.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/StringUtils.h"
#include "calwf/utils/Logger.h"

#include <algorithm>
#include <unordered_map>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 解析单个 BAWE CSV，填充 Document
// ------------------------------------------------------------
// BAWE CSV 列头：
//   sentence_number, word_number, word, lemma, pos, ner, dep_on, dep
// ------------------------------------------------------------
bool BaweImporter::loadOneCsv(const std::string& csv_path,
                              const BaweMetadataEntry& meta,
                              Document& out) {
    std::vector<std::string> header;
    auto rows = CSVParser::parseFileWithHeader(csv_path, header);

    if (header.empty() || rows.empty()) {
        return false;
    }

    // 定位列索引
    int idx_sent = -1, idx_word = -1, idx_word_id = -1;
    int idx_lemma = -1, idx_pos = -1, idx_ner = -1;
    int idx_dep_on = -1, idx_dep = -1;

    for (std::size_t i = 0; i < header.size(); ++i) {
        const std::string& h = header[i];
        if      (h == "sentence_number") idx_sent    = static_cast<int>(i);
        else if (h == "word_number")     idx_word_id = static_cast<int>(i);
        else if (h == "word")            idx_word    = static_cast<int>(i);
        else if (h == "lemma")           idx_lemma   = static_cast<int>(i);
        else if (h == "pos")             idx_pos     = static_cast<int>(i);
        else if (h == "ner")             idx_ner     = static_cast<int>(i);
        else if (h == "dep_on")          idx_dep_on  = static_cast<int>(i);
        else if (h == "dep")             idx_dep     = static_cast<int>(i);
    }

    if (idx_sent < 0 || idx_word < 0) {
        return false;
    }

    // ---- 填充元数据 ----
    out.doc_id             = "BAWE_" + meta.id;
    out.source             = SourceType::BAWE;
    out.genre              = meta.genre_family;
    out.genre_family       = meta.genre_family;
    out.level              = meta.level;
    out.discipline         = meta.discipline;
    out.disciplinary_group = meta.disciplinary_group;
    out.l1                 = meta.l1;
    out.gender             = meta.gender;
    out.year_of_birth      = meta.year_of_birth;
    out.education          = meta.education;

    // ---- 逐行读词 ----
    // BAWE CSV 是按句子分组，每个句子内部按 word_number 递增
    // 我们顺序读入 token，遇到新的 sentence_number 就开一个新句子
    int current_sent_id = -1;
    Sentence current_sent;
    std::vector<Sentence> sentences;

    auto getCol = [&](const std::vector<std::string>& row, int idx) -> std::string {
        if (idx < 0 || static_cast<std::size_t>(idx) >= row.size()) return "";
        return row[static_cast<std::size_t>(idx)];
    };

    for (const auto& row : rows) {
        if (row.empty()) continue;

        int sent_id = utils::toInt(getCol(row, idx_sent), -1);
        if (sent_id < 0) continue;

        // 新句子
        if (sent_id != current_sent_id) {
            if (!current_sent.tokens.empty()) {
                // 拼接句子文本
                std::string text;
                for (std::size_t k = 0; k < current_sent.tokens.size(); ++k) {
                    if (k > 0) text += ' ';
                    text += current_sent.tokens[k].word;
                }
                current_sent.text = text;
                sentences.push_back(std::move(current_sent));
            }
            current_sent = Sentence{};
            current_sent.id = sent_id;
            current_sent_id = sent_id;
        }

        Token tok;
        tok.sentence_id = sent_id;
        tok.word_id     = utils::toInt(getCol(row, idx_word_id), 0);
        tok.word        = getCol(row, idx_word);
        tok.lemma       = getCol(row, idx_lemma);
        tok.pos         = getCol(row, idx_pos);
        tok.ner         = getCol(row, idx_ner);
        tok.dep_on      = utils::toInt(getCol(row, idx_dep_on), -1);
        tok.dep         = getCol(row, idx_dep);

        if (!tok.word.empty()) {
            current_sent.tokens.push_back(std::move(tok));
        }
    }

    // 最后一句话
    if (!current_sent.tokens.empty()) {
        std::string text;
        for (std::size_t k = 0; k < current_sent.tokens.size(); ++k) {
            if (k > 0) text += ' ';
            text += current_sent.tokens[k].word;
        }
        current_sent.text = text;
        sentences.push_back(std::move(current_sent));
    }

    if (sentences.empty()) {
        return false;
    }

    // ---- 段落切分 ----
    // BAWE CSV 没有显式段落，粗略策略：
    //   每 5 句组成一个"段落"，最后不足 5 句也自成一段。
    // 这样保留结构，又避免单段太长。
    // 后续如需更精细，可以从 TXT 版本或 XML 版本提取段落。
    const int SENTS_PER_PARA = 5;
    Paragraph para;
    para.id = 1;
    for (std::size_t i = 0; i < sentences.size(); ++i) {
        para.sentences.push_back(std::move(sentences[i]));

        if (static_cast<int>(para.sentences.size()) >= SENTS_PER_PARA ||
            i + 1 == sentences.size()) {
            // 拼接段落文本
            std::string ptext;
            for (std::size_t k = 0; k < para.sentences.size(); ++k) {
                if (k > 0) ptext += ' ';
                ptext += para.sentences[k].text;
            }
            para.text = ptext;
            out.paragraphs.push_back(std::move(para));
            para = Paragraph{};
            para.id = static_cast<int>(out.paragraphs.size()) + 1;
        }
    }

    // ---- 拼全文 + 统计 ----
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
// 只加载元数据（调试用）
// ------------------------------------------------------------
BaweMetadataTable
BaweImporter::loadMetadata(const std::string& metadata_path) {
    BaweMetadataTable table;
    table.loadFromFile(metadata_path);
    return table;
}

// ------------------------------------------------------------
// 主入口
// ------------------------------------------------------------
std::vector<Document>
BaweImporter::load(const std::string& metadata_path,
                   const std::string& csv_root_dir) {
    stats_ = BaweImportStats{};

    // ---- 1. 加载元数据 ----
    BaweMetadataTable table;
    if (!table.loadFromFile(metadata_path)) {
        CALWF_LOG_ERROR("BAWE 导入失败：元数据加载失败");
        return {};
    }
    stats_.total_meta = static_cast<int>(table.size());

    // ---- 2. 遍历所有 CSV ----
    auto csv_files = utils::listFilesRecursive(csv_root_dir, "csv");
    stats_.total_csv = static_cast<int>(csv_files.size());

    CALWF_LOG_INFO("BAWE 导入：发现 %d 个 CSV 文件",
                   stats_.total_csv);

    std::vector<Document> result;
    result.reserve(static_cast<std::size_t>(stats_.total_csv));

    // 记录哪些 id 被 CSV 覆盖了
    std::unordered_map<std::string, bool> id_covered;

    for (const auto& csv_path : csv_files) {
        std::string id = utils::fileNameNoExt(csv_path);
        if (id.empty()) continue;

        const BaweMetadataEntry* meta = table.find(id);
        if (meta == nullptr) {
            ++stats_.skipped_no_meta;
            continue;
        }

        Document doc;
        if (!loadOneCsv(csv_path, *meta, doc)) {
            ++stats_.skipped_parse;
            continue;
        }

        if (doc.empty()) {
            ++stats_.skipped_empty;
            continue;
        }

        id_covered[id] = true;
        result.push_back(std::move(doc));
        ++stats_.imported;

        // 每 500 篇打一次进度
        if (stats_.imported % 500 == 0) {
            CALWF_LOG_INFO("BAWE 导入进度：已导入 %d 篇",
                           stats_.imported);
        }
    }

    // ---- 3. 统计"有元数据但没 CSV"的数量 ----
    for (const auto& id : table.allIds()) {
        if (id_covered.find(id) == id_covered.end()) {
            ++stats_.skipped_no_csv;
        }
    }

    CALWF_LOG_INFO("BAWE 导入完成：成功 %d 篇，"
                   "CSV 无元数据 %d，元数据无 CSV %d，"
                   "解析失败 %d，空文档 %d",
                   stats_.imported,
                   stats_.skipped_no_meta,
                   stats_.skipped_no_csv,
                   stats_.skipped_parse,
                   stats_.skipped_empty);

    return result;
}

} // namespace io
} // namespace calwf