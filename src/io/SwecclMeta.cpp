// ============================================================
// CALWF - SWECCL 标签解析实现
// ============================================================

#include "calwf/io/SwecclMeta.h"
#include "calwf/utils/StringUtils.h"

#include <cctype>
#include <unordered_set>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 已知院校代码
// ------------------------------------------------------------
static const std::unordered_set<std::string>& schoolCodes() {
    static const std::unordered_set<std::string> s = {
        "FYSY", "HZKD", "JND", "ND", "NGD", "NGG",
        "SD", "XSD", "NGSD"
    };
    return s;
}

// ------------------------------------------------------------
// 解析标签
// ------------------------------------------------------------
bool SwecclTagParser::parse(const std::string& line, SwecclTags& tags) {
    if (line.empty()) return false;

    // 去掉开头的所有 BOM 字符和空白
    std::size_t start_scan = 0;
    while (start_scan + 2 < line.size() &&
           static_cast<unsigned char>(line[start_scan]) == 0xEF &&
           static_cast<unsigned char>(line[start_scan + 1]) == 0xBB &&
           static_cast<unsigned char>(line[start_scan + 2]) == 0xBF) {
        start_scan += 3;
    }
    while (start_scan < line.size() &&
           (line[start_scan] == ' ' || line[start_scan] == '\t')) {
        ++start_scan;
    }

    if (start_scan >= line.size() || line[start_scan] != '<') {
        return false;
    }

    std::size_t pos = start_scan;
    const std::size_t n = line.size();

    while (pos < n) {
        // 找到 <
        std::size_t start = line.find('<', pos);
        if (start == std::string::npos) break;

        std::size_t end = line.find('>', start);
        if (end == std::string::npos) break;

        std::string tag = line.substr(start + 1, end - start - 1);
        std::string upper = utils::toUpper(tag);

        // WCOMP
        if (upper == "WCOMP") {
            tags.is_wcomp = true;
        }
        // 文体
        else if (upper == "ARG" || upper == "EXP" || upper == "NAR") {
            tags.genre = upper;
        }
        // 年级
        else if (utils::startsWith(upper, "GRADE")) {
            std::string num = upper.substr(5);
            if (num.size() == 1 && num[0] >= '1' && num[0] <= '4') {
                tags.grade = num[0] - '0';
            }
        }
        // 年份
        else if (utils::startsWith(upper, "YEAR")) {
            tags.year_code = upper.substr(4);
        }
        // 限时
        else if (upper == "TIMED") {
            tags.is_timed = true;
        }
        else if (upper == "UNTIMED") {
            tags.is_untimed = true;
        }
        // 分数
        else if (utils::startsWith(upper, "SCORE")) {
            tags.has_score = true;
        }
        // 长度
        else if (utils::startsWith(upper, "LENGTH")) {
            std::string s = upper.substr(6);
            // 形如 "300W" 或 "300"
            std::string num;
            for (char c : s) {
                if (c >= '0' && c <= '9') num.push_back(c);
                else break;
            }
            if (!num.empty()) {
                tags.target_length = utils::toInt(num, -1);
            }
        }
        // 院校代码
        else if (schoolCodes().count(upper) > 0) {
            tags.school_code = upper;
        }

        pos = end + 1;
    }

    return true;
}

// ------------------------------------------------------------
// 清洗正文
// ------------------------------------------------------------
// 目标：
//   <gr-come>          → 删除
//   <sp-transportation> → 删除
//   <mis-and>          → 删除
//   <a>                → 删除
//   <don't>            → 删除
// 即：删除所有 <...> 形式的标注，保留空白结构。
// ------------------------------------------------------------
std::string SwecclTagParser::cleanText(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    std::size_t i = 0;
    const std::size_t n = text.size();

    while (i < n) {
        // 跳过 BOM 字符（EF BB BF）
        if (i + 2 < n &&
            static_cast<unsigned char>(text[i]) == 0xEF &&
            static_cast<unsigned char>(text[i + 1]) == 0xBB &&
            static_cast<unsigned char>(text[i + 2]) == 0xBF) {
            i += 3;
            continue;
        }

        char c = text[i];
        if (c == '<') {
            // 找对应的 >
            std::size_t end = text.find('>', i);
            if (end == std::string::npos) {
                result.push_back(c);
                ++i;
            } else {
                i = end + 1;
            }
        } else {
            result.push_back(c);
            ++i;
        }
    }

    // 去掉多余空格
    std::string cleaned;
    cleaned.reserve(result.size());
    bool prev_space = false;
    for (char c : result) {
        if (c == ' ' || c == '\t') {
            if (!prev_space) {
                cleaned.push_back(' ');
                prev_space = true;
            }
        } else {
            cleaned.push_back(c);
            prev_space = false;
        }
    }

    return cleaned;
}

} // namespace io
} // namespace calwf