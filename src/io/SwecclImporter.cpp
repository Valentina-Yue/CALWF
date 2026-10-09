// ============================================================
// CALWF - SWECCL 导入器实现
// ============================================================

#include "calwf/io/SwecclImporter.h"
#include "calwf/io/SwecclMeta.h"
#include "calwf/text/SentenceSplitter.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/StringUtils.h"
#include "calwf/utils/Logger.h"
#include <algorithm>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 导入单个文件
// ------------------------------------------------------------
// 说明：
//   SWECCL 文件的首行格式不统一，可能是：
//     1. 首行是标签（理想情况）
//     2. 首行是文件名（如 A101NGD），第 2 行才是标签
//     3. 首行是空行
//     4. 标签行前面有多个 BOM 字符
//   因此这里改为：扫描前 10 行，找第一个以 '<' 开头且包含 "WCOMP" 的行作为标签行。
// ------------------------------------------------------------
bool SwecclImporter::loadOneFile(const std::string& path, Document& out) {
    auto lines = utils::readLines(path);
    if (lines.empty()) {
        return false;
    }

    // ---- 1. 在前 10 行里找标签行 ----
    // SWECCL 文件首行有 4 种情况：
    //   A. 直接是 <WCOMP>... 标签
    //   B. 首行是文件名（如 A101NGD），第二行才是标签
    //   C. 首行是空行，第二行才是标签
    //   D. 首行有乱码或不可见字符，后面才是标签
    // 策略：扫描前 10 行，找到第一个"以 < 开头且能成功解析"的行作为标签行。
    SwecclTags  tags;
    std::size_t tag_line = static_cast<std::size_t>(-1);

    const std::size_t max_scan = std::min<std::size_t>(10, lines.size());
    for (std::size_t i = 0; i < max_scan; ++i) {
        std::string line = lines[i];

        // 去掉 UTF-8 BOM
        if (line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
        }

        // 去掉左侧空白
        line = utils::trimLeft(line);

        // 有些文件前面有乱码（如 ����），需要找到第一个 '<' 的位置
        std::size_t lt_pos = line.find('<');
        if (lt_pos == std::string::npos) {
            continue;
        }

        // 从第一个 '<' 开始截取
        line = line.substr(lt_pos);

        // 尝试解析
        SwecclTags candidate;
        if (SwecclTagParser::parse(line, candidate)) {
            // 必须包含 <WCOMP> 才算有效标签行
            if (candidate.is_wcomp) {
                tags = candidate;
                tag_line = i;
                break;
            }
        }
    }

    if (tag_line == static_cast<std::size_t>(-1)) {
        return false;
    }


    // ---- 2. 拼装正文（标签行之后的所有内容） ----
    std::string body;
    body.reserve(4096);
    for (std::size_t i = tag_line + 1; i < lines.size(); ++i) {
        if (i > tag_line + 1) body += '\n';
        body += lines[i];
    }

    if (body.empty()) {
        return false;
    }

    // ---- 3. 清洗 ----
    std::string cleaned = SwecclTagParser::cleanText(body);

    // ---- 4. 填充 Document ----
    std::string id = utils::fileNameNoExt(path);
    out.doc_id = "SWECCL_" + id;
    out.source = SourceType::SWECCL;

    out.genre       = tags.genre;
    out.level       = tags.grade;
    out.school_code = tags.school_code;
    out.timed       = tags.is_timed;

    // ---- 5. 分句、分段 ----
    auto paragraphs = text::SentenceSplitter::splitParagraphs(cleaned);

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

    // ---- 6. 全文 + 统计 ----
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
SwecclImporter::load(const std::string& root_dir) {
    stats_ = SwecclImportStats{};

    auto txt_files = utils::listFilesRecursive(root_dir, "txt");
    stats_.total_files = static_cast<int>(txt_files.size());

    CALWF_LOG_INFO("SWECCL 导入：发现 %d 个 txt 文件",
                   stats_.total_files);

    std::vector<Document> result;
    result.reserve(static_cast<std::size_t>(stats_.total_files));

    for (const auto& path : txt_files) {
        // 跳过 tree.txt 这类非作文文件
        std::string fname = utils::fileName(path);
        if (fname == "tree.txt") continue;

        Document doc;
        if (!loadOneFile(path, doc)) {
            ++stats_.skipped_parse;
            // 记录前 20 个失败路径
            if (stats_.failed_paths.size() < 20) {
                stats_.failed_paths.push_back(path);
            }
            CALWF_LOG_WARN("导入失败：%s", path.c_str());
            continue;
        }

        if (doc.empty()) {
            ++stats_.skipped_empty;
            continue;
        }

        // 分类统计
        if      (doc.genre == "ARG") ++stats_.arg;
        else if (doc.genre == "EXP") ++stats_.exp;
        else if (doc.genre == "NAR") ++stats_.nar;
        else                         ++stats_.other;

        result.push_back(std::move(doc));
        ++stats_.imported;

        if (stats_.imported % 500 == 0) {
            CALWF_LOG_INFO("SWECCL 导入进度：%d 篇", stats_.imported);
        }
    }

    CALWF_LOG_INFO("SWECCL 导入完成：成功 %d 篇，"
                   "解析失败 %d，空文档 %d",
                   stats_.imported,
                   stats_.skipped_parse,
                   stats_.skipped_empty);

    return result;
}

// ------------------------------------------------------------
// 只导入某个子目录
// ------------------------------------------------------------
std::vector<Document>
SwecclImporter::loadSubdir(const std::string& subdir) {
    stats_ = SwecclImportStats{};

    auto txt_files = utils::listFilesRecursive(subdir, "txt");
    stats_.total_files = static_cast<int>(txt_files.size());

    std::vector<Document> result;

    for (const auto& path : txt_files) {
        std::string fname = utils::fileName(path);
        if (fname == "tree.txt") continue;

        Document doc;
        if (!loadOneFile(path, doc)) {
            ++stats_.skipped_parse;
            continue;
        }

        if (doc.empty()) {
            ++stats_.skipped_empty;
            continue;
        }

        if      (doc.genre == "ARG") ++stats_.arg;
        else if (doc.genre == "EXP") ++stats_.exp;
        else if (doc.genre == "NAR") ++stats_.nar;
        else                         ++stats_.other;

        result.push_back(std::move(doc));
        ++stats_.imported;
    }

    return result;
}

} // namespace io
} // namespace calwf