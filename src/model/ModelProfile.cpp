// ============================================================
// CALWF - ModelProfile 实现
// ============================================================

#include "calwf/model/ModelProfile.h"
#include "calwf/features/SurfaceFeatures.h"
#include "calwf/features/LexicalFeatures.h"
#include "calwf/features/SyntacticFeatures.h"
#include "calwf/features/DiscourseFeatures.h"
#include "calwf/features/SafetyFeatures.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <map>

namespace calwf {
namespace model {

using namespace calwf::features;

// ------------------------------------------------------------
// 提取一个文档的完整特征向量（不含锚定，因为锚定依赖全局状态）
// ------------------------------------------------------------
static FeatureVector extractFeatures(const Document& doc) {
    FeatureVector fv;
    SurfaceFeatures::extract(doc, fv);
    LexicalFeatures::extract(doc, fv);
    SyntacticFeatures::extract(doc, fv);
    DiscourseFeatures::extract(doc, fv);
    SafetyFeatures::extract(doc, fv);
    return fv;
}

// ------------------------------------------------------------
// 构建模型指纹
// ------------------------------------------------------------
std::vector<ModelProfile>
FingerprintBuilder::build(const std::vector<Document>& llm_docs) {
    // 1. 按 model_name 分组
    std::map<std::string, std::vector<FeatureVector>> groups;
    for (const auto& d : llm_docs) {
        if (d.model_name.empty()) continue;
        groups[d.model_name].push_back(extractFeatures(d));
    }

    // 2. 对每组计算均值和标准差
    std::vector<ModelProfile> profiles;
    profiles.reserve(groups.size());

    for (const auto& kv : groups) {
        const std::string& name = kv.first;
        const auto& fvs = kv.second;

        ModelProfile p;
        p.model_name   = name;
        p.sample_count = static_cast<int>(fvs.size());

        // 保留所有样本向量（kNN 归因用）
        p.samples = fvs;

        if (fvs.empty()) {
            profiles.push_back(p);
            continue;
        }

        // 所有特征向量拼成矩阵：n × 32
        // 用 toVector() 得到列向量，逐列统计
        std::vector<std::vector<double>> matrix;
        matrix.reserve(fvs.size());
        for (const auto& fv : fvs) {
            matrix.push_back(fv.toVector());
        }

        const std::size_t n_feat = matrix[0].size();
        const std::size_t n_doc  = matrix.size();

        std::vector<double> means(n_feat, 0.0);
        std::vector<double> stds(n_feat, 0.0);

        // 均值
        for (std::size_t i = 0; i < n_doc; ++i) {
            for (std::size_t j = 0; j < n_feat; ++j) {
                means[j] += matrix[i][j];
            }
        }
        for (std::size_t j = 0; j < n_feat; ++j) {
            means[j] /= static_cast<double>(n_doc);
        }

        // 标准差
        for (std::size_t i = 0; i < n_doc; ++i) {
            for (std::size_t j = 0; j < n_feat; ++j) {
                double d = matrix[i][j] - means[j];
                stds[j] += d * d;
            }
        }
        for (std::size_t j = 0; j < n_feat; ++j) {
            stds[j] = std::sqrt(stds[j] / static_cast<double>(n_doc));
        }

        p.mean   = FeatureVector::fromVector(means);
        p.stddev = FeatureVector::fromVector(stds);

        profiles.push_back(std::move(p));
    }

    return profiles;
}

// ------------------------------------------------------------
// 打印指纹概览
// ------------------------------------------------------------
void FingerprintBuilder::print(const std::vector<ModelProfile>& profiles) {
    std::cout << "\n";
    std::cout << "  " << std::string(10, ' ') << " | "
              << "样本  平均句长  长词率    TTR    连接词   风险词\n";
    std::cout << "  " << std::string(76, '-') << "\n";

    for (const auto& p : profiles) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "  %-10s | %4d  %8.2f  %7.4f  %6.4f  %7.4f  %7.4f",
            p.model_name.c_str(),
            p.sample_count,
            p.mean.avg_sentence_length,
            p.mean.long_word_ratio,
            p.mean.ttr,
            p.mean.connective_density,
            p.mean.risk_word_ratio);
        std::cout << buf << "\n";
    }
    std::cout << "\n";
}

} // namespace model
} // namespace calwf