// ============================================================
// CALWF - Analyzer 分析引擎
// ============================================================
//
// 文件名：include/calwf/analysis/Analyzer.h
// 作用：把 FeatureVector、ModelProfile、AnchoringFeatures、
//       SafetyFeatures 组合成一个 AnalysisResult。
//
// 核心逻辑：
//   1. 提取文档的完整特征向量
//   2. 计算 SWECCL/BAWE 锚定分
//   3. 计算与 5 个模型均值的加权欧氏距离，得到 top-3
//   4. 综合判断 AI 嫌疑度、人味指数
//   5. 收集证据（模板句、风险词、相似文档）
//
// 距离度量：
//   加权欧氏距离。每个特征用 5 个模型的 stddev 作为分母
//   （标准化），避免量级差异导致距离被大数值主导。
// ============================================================

#ifndef CALWF_ANALYSIS_ANALYZER_H
#define CALWF_ANALYSIS_ANALYZER_H

#include <string>
#include <vector>

#include "calwf/analysis/AnalysisResult.h"
#include "calwf/core/Corpus.h"
#include "calwf/features/CorpusAnchor.h"
#include "calwf/model/ModelProfile.h"

namespace calwf {
namespace analysis {

class Analyzer {
public:
    Analyzer() = default;

    // 设置参考数据
    void setModelProfiles(const std::vector<model::ModelProfile>& profiles);

    void setAnchors(const features::CorpusAnchor* sweccl,
                    const features::CorpusAnchor* bawe);

    // 设置参考语料（用于找最相似文档）
    void setReferenceCorpora(const std::vector<Document>* sweccl,
                             const std::vector<Document>* bawe);

    // 分析单个文档
    AnalysisResult analyze(const Document& doc) const;

private:
    std::vector<model::ModelProfile> model_profiles_;

    const features::CorpusAnchor* sweccl_anchor_ = nullptr;
    const features::CorpusAnchor* bawe_anchor_   = nullptr;

    const std::vector<Document>* sweccl_docs_ = nullptr;
    const std::vector<Document>* bawe_docs_   = nullptr;

    // ---- 内部计算 ----
    // 加权欧氏距离（用模型 stddev 作为分母）
    double weightedDistance(const features::FeatureVector& a,
                            const features::FeatureVector& b) const;

    // 找最相似的 N 个文档（按余弦相似度）
    std::vector<std::string> topSimilarDocs(
        const Document& doc,
        const std::vector<Document>& pool,
        std::size_t top_n) const;

    // 找最异常的句子
    std::vector<std::string> findAnomalousSentences(
        const Document& doc,
        std::size_t top_n) const;

    // kNN 归因
    // 返回：按投票数排序的 (model, votes) 列表
    std::vector<std::pair<std::string, double>> knnAttribute(
        const features::FeatureVector& fv,
        std::size_t k = 5) const;
};

} // namespace analysis
} // namespace calwf

#endif // CALWF_ANALYSIS_ANALYZER_H