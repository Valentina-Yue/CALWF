// ============================================================
// CALWF - CSVParser 实现
// ============================================================
//
// 解析算法（状态机）：
//
//   状态：
//     FIELD_START     字段开始
//     IN_UNQUOTED     在无引号字段中
//     IN_QUOTED       在引号包围的字段中
//     QUOTE_IN_QUOTED 在引号字段中遇到一个引号（可能是转义或结束）
//
//   规则：
//     - 字段开始时遇到 " 进入 IN_QUOTED
//     - IN_QUOTED 中遇到 " 进入 QUOTE_IN_QUOTED
//     - QUOTE_IN_QUOTED 中遇到另一个 " 则追加一个 " 并回到 IN_QUOTED
//     - QUOTE_IN_QUOTED 中遇到分隔符则结束字段
//     - 分隔符结束当前字段并开始下一个
//     - \n 结束当前行并开始下一行（但 IN_QUOTED 内的 \n 是字段内容）
// ============================================================

#include "calwf/io/CSVParser.h"
#include "calwf/utils/FileUtils.h"

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 核心解析函数
// ------------------------------------------------------------
std::vector<std::vector<std::string>>
CSVParser::parseString(const std::string& content, char delim) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;

    bool in_quotes = false;
    bool field_started = false;

    const std::size_t n = content.size();
    std::size_t i = 0;

    // ---- 跳过 UTF-8 BOM（EF BB BF） ----
    // 有些 CSV 文件带 BOM，直接解析会污染第一个字段。
    // 这里统一在解析前跳过。
    if (n >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        i = 3;
    }

    while (i < n) {
        char c = content[i];

        if (in_quotes) {
            // 在引号包围的字段中
            if (c == '"') {
                // 可能是结束引号，也可能是转义 ""
                if (i + 1 < n && content[i + 1] == '"') {
                    // 转义，追加一个引号
                    field += '"';
                    i += 2;
                    continue;
                } else {
                    // 结束引号
                    in_quotes = false;
                    ++i;
                    continue;
                }
            } else {
                // 引号内的任意字符（包括逗号、换行）都是字段内容
                field += c;
                ++i;
                continue;
            }
        } else {
            // 不在引号中
            if (c == '"' && !field_started) {
                // 字段开始的引号
                in_quotes = true;
                field_started = true;
                ++i;
                continue;
            } else if (c == delim) {
                // 字段结束
                row.push_back(field);
                field.clear();
                field_started = false;
                ++i;
                continue;
            } else if (c == '\r') {
                // 行尾可能是 \r\n 或 \r
                row.push_back(field);
                field.clear();
                field_started = false;
                rows.push_back(std::move(row));
                row.clear();
                ++i;
                if (i < n && content[i] == '\n') {
                    ++i;
                }
                continue;
            } else if (c == '\n') {
                row.push_back(field);
                field.clear();
                field_started = false;
                rows.push_back(std::move(row));
                row.clear();
                ++i;
                continue;
            } else {
                field += c;
                field_started = true;
                ++i;
                continue;
            }
        }
    }

    // 处理最后一个字段和最后一行
    if (!field.empty() || !row.empty() || field_started) {
        row.push_back(field);
    }
    if (!row.empty()) {
        rows.push_back(std::move(row));
    }

    return rows;
}

// ------------------------------------------------------------
// 文件解析
// ------------------------------------------------------------
std::vector<std::vector<std::string>>
CSVParser::parseFile(const std::string& path, char delim) {
    std::string content = utils::readFile(path);
    if (content.empty()) return {};
    return parseString(content, delim);
}

// ------------------------------------------------------------
// 带表头解析
// ------------------------------------------------------------
std::vector<std::vector<std::string>>
CSVParser::parseStringWithHeader(const std::string& content,
                                 std::vector<std::string>& header,
                                 char delim) {
    auto all = parseString(content, delim);
    if (all.empty()) {
        header.clear();
        return {};
    }
    header = all.front();
    all.erase(all.begin());
    return all;
}

std::vector<std::vector<std::string>>
CSVParser::parseFileWithHeader(const std::string& path,
                               std::vector<std::string>& header,
                               char delim) {
    std::string content = utils::readFile(path);
    if (content.empty()) {
        header.clear();
        return {};
    }
    return parseStringWithHeader(content, header, delim);
}

} // namespace io
} // namespace calwf