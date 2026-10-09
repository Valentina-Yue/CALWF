// ============================================================
// CALWF - FileUtils 文件工具
// ============================================================
//
// 文件名：include/calwf/utils/FileUtils.h
// 作用：读写文件、遍历目录、路径处理。
//
// 对应数据结构课程知识：
//   - 串（路径拼接）
//   - 线性表（vector 存储行、路径）
// ============================================================

#ifndef CALWF_UTILS_FILEUTILS_H
#define CALWF_UTILS_FILEUTILS_H

#include <string>
#include <vector>

namespace calwf {
namespace utils {

// ============ 文件读写 ============

// 读取整个文件为字符串（UTF-8）
// 失败返回空字符串
std::string readFile(const std::string& path);

// 按行读取文件
// 自动去除行尾的 \r 和 \n
// 失败返回空 vector
std::vector<std::string> readLines(const std::string& path);

// 写入整个字符串到文件（UTF-8）
// 返回是否成功
bool writeFile(const std::string& path, const std::string& content);

// 按行写入
bool writeLines(const std::string& path,
                const std::vector<std::string>& lines);

// 追加写入
bool appendFile(const std::string& path, const std::string& content);

// ============ 文件系统 ============

// 文件是否存在
bool fileExists(const std::string& path);

// 目录是否存在
bool dirExists(const std::string& path);

// 创建目录（不递归）
bool createDir(const std::string& path);

// 递归创建目录
bool createDirs(const std::string& path);

// 列出目录下的所有文件（不递归）
// 返回完整路径
std::vector<std::string> listFiles(const std::string& dir,
                                   const std::string& extension = "");

// 递归列出目录下的所有文件
std::vector<std::string> listFilesRecursive(const std::string& dir,
                                            const std::string& extension = "");

// ============ 路径处理 ============

// 取文件名（含扩展名）
std::string fileName(const std::string& path);

// 取文件名（不含扩展名）
std::string fileNameNoExt(const std::string& path);

// 取扩展名（不含点）
std::string fileExtension(const std::string& path);

// 取目录部分
std::string dirName(const std::string& path);

// 拼接路径（自动加分隔符）
std::string joinPath(const std::string& a, const std::string& b);

} // namespace utils
} // namespace calwf

#endif // CALWF_UTILS_FILEUTILS_H