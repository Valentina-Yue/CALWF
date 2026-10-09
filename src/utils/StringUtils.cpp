// ============================================================
// CALWF - StringUtils 实现
// ============================================================

#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace calwf {
namespace utils {

// ============ 基础操作 ============

std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

std::string toUpper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return r;
}

std::string trimLeft(const std::string& s) {
    std::size_t i = 0;
    while (i < s.size() && isWhitespace(s[i])) ++i;
    return s.substr(i);
}

std::string trimRight(const std::string& s) {
    if (s.empty()) return s;
    std::size_t i = s.size();
    while (i > 0 && isWhitespace(s[i - 1])) --i;
    return s.substr(0, i);
}

std::string trim(const std::string& s) {
    return trimRight(trimLeft(s));
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

std::string replaceAll(const std::string& s,
                       const std::string& oldStr,
                       const std::string& newStr) {
    if (oldStr.empty()) return s;
    std::string result;
    result.reserve(s.size());
    std::size_t pos = 0;
    while (pos < s.size()) {
        std::size_t found = s.find(oldStr, pos);
        if (found == std::string::npos) {
            result.append(s, pos, std::string::npos);
            break;
        }
        result.append(s, pos, found - pos);
        result.append(newStr);
        pos = found + oldStr.size();
    }
    return result;
}

// ============ 分割与合并 ============

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::string cur;
    for (char c : s) {
        if (c == delim) {
            if (!cur.empty()) {
                result.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        result.push_back(cur);
    }
    return result;
}

std::vector<std::string> split(const std::string& s,
                               const std::string& delim) {
    std::vector<std::string> result;
    if (delim.empty()) {
        result.push_back(s);
        return result;
    }
    std::size_t pos = 0;
    while (pos <= s.size()) {
        std::size_t found = s.find(delim, pos);
        if (found == std::string::npos) {
            result.push_back(s.substr(pos));
            break;
        }
        result.push_back(s.substr(pos, found - pos));
        pos = found + delim.size();
    }
    return result;
}

std::string join(const std::vector<std::string>& parts,
                 const std::string& delim) {
    std::string result;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += delim;
        result += parts[i];
    }
    return result;
}

// ============ 字符判断 ============

bool isAlpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isAlnum(char c) {
    return isAlpha(c) || isDigit(c);
}

bool isPunctuation(char c) {
    static const std::string puncts = ".,;:!?'\"()[]{}-_/\\@#$%^&*+=<>|~`";
    return puncts.find(c) != std::string::npos;
}

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

// ============ KMP ============

// 计算 KMP 的 next 数组（最长相同前后缀长度）
static std::vector<int> buildNext(const std::string& pattern) {
    std::vector<int> next(pattern.size(), 0);
    for (std::size_t i = 1, j = 0; i < pattern.size(); ++i) {
        while (j > 0 && pattern[i] != pattern[j]) {
            j = static_cast<std::size_t>(next[j - 1]);
        }
        if (pattern[i] == pattern[j]) ++j;
        next[i] = static_cast<int>(j);
    }
    return next;
}

std::vector<std::size_t> kmpFindAll(const std::string& text,
                                    const std::string& pattern) {
    std::vector<std::size_t> result;
    if (pattern.empty() || text.size() < pattern.size()) return result;

    std::vector<int> next = buildNext(pattern);
    std::size_t j = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        while (j > 0 && text[i] != pattern[j]) {
            j = static_cast<std::size_t>(next[j - 1]);
        }
        if (text[i] == pattern[j]) ++j;
        if (j == pattern.size()) {
            result.push_back(i - j + 1);
            j = static_cast<std::size_t>(next[j - 1]);
        }
    }
    return result;
}

bool kmpContains(const std::string& text, const std::string& pattern) {
    if (pattern.empty()) return true;
    if (text.size() < pattern.size()) return false;

    std::vector<int> next = buildNext(pattern);
    std::size_t j = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        while (j > 0 && text[i] != pattern[j]) {
            j = static_cast<std::size_t>(next[j - 1]);
        }
        if (text[i] == pattern[j]) ++j;
        if (j == pattern.size()) return true;
    }
    return false;
}

// ============ 动态规划 ============

int editDistance(const std::string& a, const std::string& b) {
    // 保证 a 是较短的
    if (a.size() > b.size()) return editDistance(b, a);

    const std::size_t m = a.size();
    const std::size_t n = b.size();

    std::vector<int> prev(m + 1), curr(m + 1);
    for (std::size_t i = 0; i <= m; ++i) prev[i] = static_cast<int>(i);

    for (std::size_t j = 1; j <= n; ++j) {
        curr[0] = static_cast<int>(j);
        for (std::size_t i = 1; i <= m; ++i) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[i] = std::min({
                prev[i] + 1,        // 删除
                curr[i - 1] + 1,    // 插入
                prev[i - 1] + cost  // 替换
            });
        }
        std::swap(prev, curr);
    }
    return prev[m];
}

int lcsLength(const std::string& a, const std::string& b) {
    const std::size_t m = a.size();
    const std::size_t n = b.size();
    std::vector<int> prev(n + 1, 0), curr(n + 1, 0);

    for (std::size_t i = 1; i <= m; ++i) {
        for (std::size_t j = 1; j <= n; ++j) {
            if (a[i - 1] == b[j - 1]) {
                curr[j] = prev[j - 1] + 1;
            } else {
                curr[j] = std::max(prev[j], curr[j - 1]);
            }
        }
        std::swap(prev, curr);
        std::fill(curr.begin(), curr.end(), 0);
    }
    return prev[n];
}

// ============ 词频统计 ============

std::vector<std::pair<std::string, int>>
countWords(const std::vector<std::string>& tokens) {
    std::unordered_map<std::string, int> freq;
    std::vector<std::string> order;
    for (const auto& t : tokens) {
        if (freq.find(t) == freq.end()) {
            order.push_back(t);
        }
        ++freq[t];
    }
    std::vector<std::pair<std::string, int>> result;
    result.reserve(order.size());
    for (const auto& w : order) {
        result.emplace_back(w, freq[w]);
    }
    return result;
}

std::vector<std::pair<std::string, int>>
countWordsLower(const std::vector<std::string>& tokens) {
    std::unordered_map<std::string, int> freq;
    std::vector<std::string> order;
    for (const auto& t : tokens) {
        std::string key = toLower(t);
        if (freq.find(key) == freq.end()) {
            order.push_back(key);
        }
        ++freq[key];
    }
    std::vector<std::pair<std::string, int>> result;
    result.reserve(order.size());
    for (const auto& w : order) {
        result.emplace_back(w, freq[w]);
    }
    return result;
}

// ============ 数值解析 ============

int toInt(const std::string& s, int defaultValue) {
    try {
        std::size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return defaultValue;
        return v;
    } catch (...) {
        return defaultValue;
    }
}

double toDouble(const std::string& s, double defaultValue) {
    try {
        std::size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos != s.size()) return defaultValue;
        return v;
    } catch (...) {
        return defaultValue;
    }
}

} // namespace utils
} // namespace calwf