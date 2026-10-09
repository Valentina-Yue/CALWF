// ============================================================
// CALWF - CSVParser CSV 解析器
// ============================================================
//
// 文件名：include/calwf/io/CSVParser.h
// 作用：解析 CSV 文件为二维字符串表格。
//
// 支持：
//   - 标准 CSV（逗号分隔）
//   - 引号包围字段，如 "Hello, World"
//   - 引号内双引号转义："" 表示一个 "
//   - 字段内换行
//   - 自定义分隔符（默认逗号）
//
// 返回结构：
//   vector<vector<string>>：外层是行，内层是列
//
// 对应数据结构课程知识：串 / 线性表 / 状态机
// ============================================================

#ifndef CALWF_IO_CSVPARSER_H
#define CALWF_IO_CSVPARSER_H

#include <string>
#include <vector>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// CSV 解析器
// ------------------------------------------------------------
class CSVParser {
public:
    // 解析 CSV 文件
    // path       —— 文件路径
    // delim      —— 分隔符，默认逗号
    // 返回：所有行，每行是一个字段列表；失败返回空
    static std::vector<std::vector<std::string>>
    parseFile(const std::string& path, char delim = ',');

    // 解析 CSV 字符串
    static std::vector<std::vector<std::string>>
    parseString(const std::string& content, char delim = ',');

    // 解析 CSV 文件，同时返回列名（第一行）
    // header 输出参数：第一行内容
    // 返回：数据行（不含表头）
    static std::vector<std::vector<std::string>>
    parseFileWithHeader(const std::string& path,
                        std::vector<std::string>& header,
                        char delim = ',');

    // 解析 CSV 字符串，同时返回列名
    static std::vector<std::vector<std::string>>
    parseStringWithHeader(const std::string& content,
                          std::vector<std::string>& header,
                          char delim = ',');
};

} // namespace io
} // namespace calwf

#endif // CALWF_IO_CSVPARSER_H