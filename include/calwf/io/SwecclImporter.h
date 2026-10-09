// ============================================================
// CALWF - SWECCL 导入器
// ============================================================
//
// 文件名：include/calwf/io/SwecclImporter.h
// 作用：把 SWECCL 笔语子库的 txt 文件读入为 vector<Document>。
//
// 输入：
//   root_dir  —— Raw Data (WECCL) 的根目录
//
// 子目录结构：
//   Argumentation/               （A101FYSY.txt 格式）
//   Years 1-4 Essays/
//     ├── Argumentation/Year 1-4/
//     ├── Exposition/Year 1-3/
//     └── Narration/Year 1-4/
//
// 流程：
//   1. 递归遍历所有 .txt
//   2. 每个文件：
//        - 首行 → 解析标签
//        - 剩余 → 清洗正文
//        - 分句、分段
//        - 组装 Document
//
// 注意：
//   Argumentation/ 中的文件名形如 A101FYSY.txt，
//   其中 A 表示 Argumentation，101 是学生编号，FYSY 是院校代码。
//   我们不做特殊解析，只用元数据字段。
// ============================================================

#ifndef CALWF_IO_SWECCLIMPORTER_H
#define CALWF_IO_SWECCLIMPORTER_H

#include <string>
#include <vector>

#include "calwf/core/Document.h"

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 导入统计
// ------------------------------------------------------------
struct SwecclImportStats {
    int total_files    = 0;
    int imported       = 0;
    int skipped_no_tag = 0;
    int skipped_empty  = 0;
    int skipped_parse  = 0;

    // 分类统计
    int arg = 0;
    int exp = 0;
    int nar = 0;
    int other = 0;

    // 前 20 个失败路径（用于排查）
    std::vector<std::string> failed_paths;
};

// ------------------------------------------------------------
// SWECCL 导入器
// ------------------------------------------------------------
class SwecclImporter {
public:
    // 从根目录导入所有 txt
    std::vector<Document> load(const std::string& root_dir);

    // 只导入一个子目录（例如只导入 Years 1-4 Essays）
    std::vector<Document> loadSubdir(const std::string& subdir);

    const SwecclImportStats& stats() const { return stats_; }

private:
    SwecclImportStats stats_;

    // 导入单个 txt 文件
    bool loadOneFile(const std::string& path, Document& out);
};

} // namespace io
} // namespace calwf

#endif // CALWF_IO_SWECCLIMPORTER_H