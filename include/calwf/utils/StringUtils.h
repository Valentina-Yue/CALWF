// ============================================================
// CALWF - StringUtils 字符串工具
// ============================================================
//
// 文件名：include/calwf/utils/StringUtils.h
// 作用：提供文本处理常用的字符串操作。
//
// 对应数据结构课程知识：
//   - 串（string）
//   - KMP 查找
//   - 动态规划（编辑距离、LCS）
//   - 排序（去重前排序）
// ============================================================

#ifndef CALWF_UTILS_STRINGUTILS_H
#define CALWF_UTILS_STRINGUTILS_H

#include <string>
#include <vector>

namespace calwf {
namespace utils {

// ============ 基础操作 ============

// 转小写（ASCII，不处理中文）
std::string toLower(const std::string& s);

// 转大写
std::string toUpper(const std::string& s);

// 去掉两端空白（空格、\t、\r、\n）
std::string trim(const std::string& s);

// 去掉左侧空白
std::string trimLeft(const std::string& s);

// 去掉右侧空白
std::string trimRight(const std::string& s);

// 是否以 prefix 开头
bool startsWith(const std::string& s, const std::string& prefix);

// 是否以 suffix 结尾
bool endsWith(const std::string& s, const std::string& suffix);

// 是否包含子串
bool contains(const std::string& s, const std::string& sub);

// 替换所有 oldStr 为 newStr
std::string replaceAll(const std::string& s,
                       const std::string& oldStr,
                       const std::string& newStr);

// ============ 分割与合并 ============

// 按单个分隔符分割（不保留空串）
std::vector<std::string> split(const std::string& s, char delim);

// 按字符串分隔符分割
std::vector<std::string> split(const std::string& s,
                               const std::string& delim);

// 用分隔符连接
std::string join(const std::vector<std::string>& parts,
                 const std::string& delim);

// ============ 字符判断 ============

// 是否是字母（A-Z a-z）
bool isAlpha(char c);

// 是否是数字（0-9）
bool isDigit(char c);

// 是否是字母或数字
bool isAlnum(char c);

// 是否是标点符号（. , ; : ! ? ' " ( ) [ ] { } - _ 等）
bool isPunctuation(char c);

// 是否是空白
bool isWhitespace(char c);

// ============ 串匹配（KMP） ============

// KMP 算法：在 text 中查找 pattern，返回所有出现的起始位置
// 时间复杂度：O(n + m)
std::vector<std::size_t> kmpFindAll(const std::string& text,
                                    const std::string& pattern);

// KMP 是否找到
bool kmpContains(const std::string& text, const std::string& pattern);

// ============ 动态规划 ============

// 编辑距离（Levenshtein Distance）
// 允许插入、删除、替换
// 时间复杂度：O(mn)，空间：O(min(m,n))
int editDistance(const std::string& a, const std::string& b);

// 最长公共子序列长度（LCS）
// 时间复杂度：O(mn)
int lcsLength(const std::string& a, const std::string& b);

// ============ 词频统计 ============

// 统计词频（区分大小写）
// 返回顺序：按词首次出现顺序
std::vector<std::pair<std::string, int>>
countWords(const std::vector<std::string>& tokens);

// 统计词频（不区分大小写，统一转小写）
std::vector<std::pair<std::string, int>>
countWordsLower(const std::vector<std::string>& tokens);

// ============ 数值解析 ============

// 转 int（失败返回 defaultValue）
int toInt(const std::string& s, int defaultValue = 0);

// 转 double（失败返回 defaultValue）
double toDouble(const std::string& s, double defaultValue = 0.0);

} // namespace utils
} // namespace calwf

#endif // CALWF_UTILS_STRINGUTILS_H