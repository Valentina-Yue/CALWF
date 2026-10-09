// ============================================================
// CALWF - BAWE 导入器
// ============================================================
//
// 文件名：include/calwf/io/BaweImporter.h
// 作用：把 BAWE 元数据表 + 逐词 CSV 整合为 vector<Document>。
//
// 输入：
//   metadata_path       —— bawe_metadata.csv
//   csv_root_dir        —— bawe_csv_categorized 根目录
//
// 流程：
//   1. 加载元数据到 BaweMetadataTable
//   2. 递归遍历 csv_root_dir 下的所有 .csv
//   3. 每个 CSV 的文件名（不含扩展名）就是文档 id
//   4. 从元数据查对应 entry
//   5. 解析 CSV，按 sentence_number 分组，形成句子
//   6. 按段落切分（BAWE CSV 没有显式段落信息，按空行/句群粗略切）
//   7. 组装 Document
//
// 注意：
//   BAWE CSV 的列头是
//     sentence_number, word_number, word, lemma, pos, ner, dep_on, dep
//   每个文档一个 CSV，文件名如 0003i.csv
// ============================================================

#ifndef CALWF_IO_BAWEIMPORTER_H
#define CALWF_IO_BAWEIMPORTER_H

#include <string>
#include <vector>

#include "calwf/core/Document.h"
#include "calwf/io/BaweMetadata.h"

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 导入结果统计
// ------------------------------------------------------------
struct BaweImportStats {
    int total_csv        = 0;   // 扫描到的 CSV 总数
    int total_meta       = 0;   // 元数据条数
    int imported         = 0;   // 成功导入
    int skipped_no_meta  = 0;   // CSV 找不到元数据
    int skipped_no_csv   = 0;   // 元数据找不到 CSV
    int skipped_parse    = 0;   // CSV 解析失败
    int skipped_empty    = 0;   // CSV 无有效行
};

// ------------------------------------------------------------
// BAWE 导入器
// ------------------------------------------------------------
class BaweImporter {
public:
    // 导入全部
    // 返回：成功导入的 Document 列表
    std::vector<Document> load(const std::string& metadata_path,
                               const std::string& csv_root_dir);

    // 只导入元数据（用于调试）
    static BaweMetadataTable loadMetadata(const std::string& metadata_path);

    // 统计
    const BaweImportStats& stats() const { return stats_; }

private:
    BaweImportStats stats_;

    // 导入单个 CSV
    // 返回 true 表示成功，结果写入 out
    bool loadOneCsv(const std::string& csv_path,
                    const BaweMetadataEntry& meta,
                    Document& out);
};

} // namespace io
} // namespace calwf

#endif // CALWF_IO_BAWEIMPORTER_H