// ============================================================
// CALWF - AnalysisResult 分析结果
// ============================================================
//
// 文件名：include/calwf/analysis/AnalysisResult.h
// 作用：定义单个文档的分析结果结构。
//
// 这是系统最终输出的东西——教师、研究者、学生看到的"报告"。
//
// 四维评分：
//   ai_suspicion      AI 嫌疑度（0-1，越高越像 AI）
//   human_likeness    人味指数（0-1，越高越像真人）
//   sweccl_anchoring  SWECCL 锚定分（0-1）
//   bawe_anchoring    BAWE 锚定分（0-1）
//
// 模型归因：
//   5 个国内模型的距离排序，取 top-3
//
// 风险：
//   Low / Medium / High + 具体原因列表
//
// 证据链：
//   模板短语命中、风险词命中、最相似真实文档、最异常句子
// ============================================================

#ifndef CALWF_ANALYSIS_ANALYSISRESULT_H
#define CALWF_ANALYSIS_ANALYSISRESULT_H

#include <string>
#include <vector>

#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace analysis {

// ------------------------------------------------------------
// 单个模型的距离得分
// ------------------------------------------------------------
struct ModelScore {
    std::string model_name;
    double      distance   = 0.0;   // 加权欧氏距离（越小越像）
    double      confidence = 0.0;   // 归一化置信度（0-1，越大越像）
};

// ------------------------------------------------------------
// 分析结果
// ------------------------------------------------------------
struct AnalysisResult {
    // ---- 基本 ----
    std::string doc_id;
    std::string source;            // "SWECCL" / "BAWE" / "LLM" / "Unknown"

    // ---- 四维评分（0-1） ----
    double ai_suspicion      = 0.0;
    double human_likeness    = 0.0;
    double sweccl_anchoring  = 0.0;
    double bawe_anchoring    = 0.0;

    // ---- 定性判断 ----
    std::string verdict;           // "AI" / "Human" / "Uncertain"

    // ---- 模型归因（top-3） ----
    std::vector<ModelScore> model_ranking;

    // ---- 风险 ----
    std::string              risk_level;   // "Low" / "Medium" / "High"
    std::vector<std::string> risk_reasons;

    // ---- 证据链 ----
    std::vector<std::string> template_hits;       // 命中的模板短语
    std::vector<std::string> risk_hits;           // 命中的风险词
    std::vector<std::string> top_sweccl_docs;     // 最相似的 SWECCL 文档
    std::vector<std::string> top_bawe_docs;       // 最相似的 BAWE 文档
    std::vector<std::string> anomalous_sentences; // 最异常的句子

    // ---- 原始特征 ----
    features::FeatureVector features;
};

} // namespace analysis
} // namespace calwf

#endif // CALWF_ANALYSIS_ANALYSISRESULT_H