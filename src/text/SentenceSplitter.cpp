// ============================================================
// CALWF - SentenceSplitter 实现
// ============================================================

#include "calwf/text/SentenceSplitter.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <unordered_set>

namespace calwf {
namespace text {

// ------------------------------------------------------------
// 常见缩写表
// ------------------------------------------------------------
static const std::unordered_set<std::string>& abbreviations() {
    static const std::unordered_set<std::string> s = {
        "mr", "mrs", "ms", "dr", "prof", "sr", "jr", "st",
        "etc", "eg", "ie", "vs", "al", "ibid", "op", "cit",
        "no", "vol", "pp", "p", "cf", "ca",
        "jan", "feb", "mar", "apr", "jun", "jul", "aug",
        "sep", "sept", "oct", "nov", "dec",
        "mon", "tue", "wed", "thu", "fri", "sat", "sun",
        "mr.", "mrs.", "dr.", "st."
    };
    return s;
}

// ------------------------------------------------------------
// 判断是否是缩写
// ------------------------------------------------------------
bool SentenceSplitter::isAbbreviation(const std::string& text,
                                      std::size_t pos) {
    // pos 指向 '.'，往前找到单词起点
    if (pos == 0) return false;

    std::size_t end = pos;
    std::size_t start = end;
    while (start > 0 && utils::isAlpha(text[start - 1])) {
        --start;
    }
    if (start == end) return false;

    std::string word = utils::toLower(text.substr(start, end - start));
    if (word.empty()) return false;

    // 长度 1 的大写字母 + '.' 视为缩写（如 "A."）
    if (word.size() == 1) return true;

    return abbreviations().count(word) > 0;
}

// ------------------------------------------------------------
// 判断位置 pos 之后是否构成句边界
// ------------------------------------------------------------
bool SentenceSplitter::isBoundary(const std::string& text,
                                  std::size_t pos) {
    // pos 指向强标点
    if (pos >= text.size()) return false;

    char p = text[pos];
    if (p != '.' && p != '!' && p != '?') return false;

    // 句号可能是缩写
    if (p == '.' && isAbbreviation(text, pos)) {
        return false;
    }

    // 看后面的字符
    std::size_t i = pos + 1;

    // 允许 0 到 2 个引号/右括号/空格
    int skip = 0;
    while (i < text.size() && skip < 3) {
        char c = text[i];
        if (c == ' ' || c == '\t') {
            ++i;
            ++skip;
        } else if (c == '"' || c == '\'' || c == ')' || c == ']' || c == '}') {
            ++i;
            ++skip;
        } else {
            break;
        }
    }

    if (i >= text.size()) {
        // 文本末尾，视为边界
        return true;
    }

    // 后面是大写字母或数字，视为边界
    char c = text[i];
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '"' || c == '\'' || c == '(') return true;

    return false;
}

// ------------------------------------------------------------
// 分句
// ------------------------------------------------------------
std::vector<std::string> SentenceSplitter::split(const std::string& text) {
    std::vector<std::string> result;
    if (text.empty()) return result;

    std::size_t start = 0;
    const std::size_t n = text.size();

    for (std::size_t i = 0; i < n; ++i) {
        if (isBoundary(text, i)) {
            // i 是标点，i+1 是切分点
            std::size_t end = i + 1;
            // 把后面的引号和空格也纳入本句
            while (end < n &&
                   (text[end] == '"' || text[end] == '\'' ||
                    text[end] == ')' || text[end] == ']')) {
                ++end;
            }
            std::string s = utils::trim(text.substr(start, end - start));
            if (!s.empty()) {
                result.push_back(std::move(s));
            }
            start = end;
            i = end - 1;
        }
    }

    // 剩余
    if (start < n) {
        std::string s = utils::trim(text.substr(start));
        if (!s.empty()) {
            result.push_back(std::move(s));
        }
    }

    return result;
}

// ------------------------------------------------------------
// 按段落切分
// ------------------------------------------------------------
// 规则（SWECCL 与常见英文作文格式）：
//   1. 空行 → 段落结束
//   2. 行首有 1 个以上空格 或 tab → 新段落开始
//   3. 第一行如果短（< 80 字符）且后面有内容 → 视为标题，作为独立段落
//   4. 其余 → 继续当前段落
// ------------------------------------------------------------
std::vector<std::vector<std::string>>
SentenceSplitter::splitParagraphs(const std::string& text) {
    std::vector<std::vector<std::string>> result;

    // ---- 1. 按 \n 切行 ----
    std::vector<std::string> lines;
    {
        std::string cur;
        for (char c : text) {
            if (c == '\n' || c == '\r') {
                lines.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) lines.push_back(cur);
    }

    // ---- 2. 检测首行是否为标题 ----
    std::size_t start = 0;
    if (!lines.empty()) {
        std::string t0 = utils::trim(lines[0]);
        bool first_line_no_indent = (!lines[0].empty() &&
                                     lines[0][0] != ' ' &&
                                     lines[0][0] != '\t');
        // 首行短（< 80 字符）且无缩进 → 标题
        if (!t0.empty() && t0.size() < 80 && first_line_no_indent) {
            auto sents = split(t0);
            if (!sents.empty()) {
                result.push_back(std::move(sents));
            }
            start = 1;
        }
    }

    // ---- 3. 处理剩余行 ----
    std::string para_buf;
    for (std::size_t i = start; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        std::string trimmed = utils::trim(line);

        if (trimmed.empty()) {
            // 空行：段落结束
            if (!para_buf.empty()) {
                auto sents = split(para_buf);
                if (!sents.empty()) {
                    result.push_back(std::move(sents));
                }
                para_buf.clear();
            }
            continue;
        }

        // 行首有缩进 → 新段落
        bool has_indent = (!line.empty() &&
                           (line[0] == ' ' || line[0] == '\t'));

        if (has_indent && !para_buf.empty()) {
            // 先把当前段 flush
            auto sents = split(para_buf);
            if (!sents.empty()) {
                result.push_back(std::move(sents));
            }
            para_buf.clear();
        }

        // 追加到当前段
        if (!para_buf.empty()) para_buf += ' ';
        para_buf += trimmed;
    }

    // 最后一段
    if (!para_buf.empty()) {
        auto sents = split(para_buf);
        if (!sents.empty()) {
            result.push_back(std::move(sents));
        }
    }

    return result;
}

} // namespace text
} // namespace calwf