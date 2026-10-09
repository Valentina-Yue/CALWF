// ============================================================
// CALWF - LLM 输出导入器
// ============================================================
//
// 文件名：include/calwf/io/LlmImporter.h
// 作用：读取 data/llm/<model>/<prompt_id>_run<N>.txt 文件，
//       并结合 llm_meta.csv 生成 vector<Document>。
//
// 输入：
//   llm_root_dir  —— data/llm 目录，子目录是 5 个模型名
//   meta_path     —— data/metadata/llm_meta.csv
//
// 处理流程：
//   1. 读取 llm_meta.csv，建立 (model, prompt_id, run) → 元数据 的映射
//   2. 遍历 llm_root_dir 下所有 .txt
//   3. 从文件名解析出 prompt_id 和 run_number
//   4. 从目录名解析出 model_name
//   5. 从元数据查对应的 topic、genre、target_level 等
//   6. 读文件内容，分句、分段
//   7. 组装 Document
//
// 文件命名规范：
//   data/llm/<ModelFolder>/<prompt_id>_run<N>.txt
//   例如：data/llm/DeepSeek/p001_run1.txt
// ============================================================

#ifndef CALWF_IO_LLMIMPORTER_H
#define CALWF_IO_LLMIMPORTER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "calwf/core/Document.h"

namespace calwf {
namespace io {

// ------------------------------------------------------------
// LLM 元数据条目
// ------------------------------------------------------------
struct LlmMetadataEntry {
    std::string doc_id;
    std::string model_name;
    std::string model_version;
    std::string prompt_id;
    std::string topic;
    std::string genre;
    std::string target_level;
    int         run_number      = -1;
    bool        deep_thinking   = false;
    bool        web_access      = false;
    std::string date_generated;
    std::string file_path;
    std::string notes;
};

// ------------------------------------------------------------
// 导入统计
// ------------------------------------------------------------
struct LlmImportStats {
    int total_files    = 0;
    int imported       = 0;
    int skipped_no_meta = 0;
    int skipped_empty  = 0;
    int skipped_parse  = 0;

    // 按模型统计
    std::unordered_map<std::string, int> per_model;
};

// ------------------------------------------------------------
// LLM 导入器
// ------------------------------------------------------------
class LlmImporter {
public:
    // 主入口
    std::vector<Document> load(const std::string& llm_root_dir,
                               const std::string& meta_path);

    const LlmImportStats& stats() const { return stats_; }

private:
    LlmImportStats stats_;

    // 加载元数据表
    bool loadMetadata(const std::string& meta_path,
                      std::unordered_map<std::string, LlmMetadataEntry>& table);

    // 导入单个文件
    bool loadOneFile(const std::string& path,
                     const LlmMetadataEntry& meta,
                     const std::string& model_name,
                     Document& out);
};

} // namespace io
} // namespace calwf

#endif // CALWF_IO_LLMIMPORTER_H