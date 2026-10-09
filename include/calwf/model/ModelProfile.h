// ============================================================
// CALWF - ModelProfile 模型指纹
// ============================================================
//
// 文件名：include/calwf/model/ModelProfile.h
// 作用：把一个模型的所有文档聚合成一个"指纹"——
//       即该模型在 32 维特征空间中的均值向量。
//
// 用途：
//   - 模型归因：对任何新文本，判断它最像哪个模型
//   - 模型比较：计算两个模型之间的风格距离
//   - 模型指纹可视化
//
// 对应数据结构课程知识：
//   - 类 / 封装
//   - 线性表（vector 存储每个模型的特征）
//   - 排序（按距离排序）
// ============================================================

#ifndef CALWF_MODEL_MODELPROFILE_H
#define CALWF_MODEL_MODELPROFILE_H

#include <string>
#include <vector>

#include "calwf/core/Document.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace model {

// ------------------------------------------------------------
// 单个模型的指纹
// ------------------------------------------------------------
struct ModelProfile {
    std::string   model_name;      // "DeepSeek" / "Doubao" / ...
    int           sample_count = 0;
    features::FeatureVector mean;      // 均值特征
    features::FeatureVector stddev;    // 标准差（用于标准化距离）
    std::vector<features::FeatureVector> samples;  // 所有样本（用于 kNN）
};

// ------------------------------------------------------------
// 指纹构建器
// ------------------------------------------------------------
class FingerprintBuilder {
public:
    // 从一组 LLM 文档构建模型指纹
    // 输入文档的 model_name 字段必须已填
    static std::vector<ModelProfile>
    build(const std::vector<Document>& llm_docs);

    // 打印指纹概览（调试用）
    static void print(const std::vector<ModelProfile>& profiles);
};

} // namespace model
} // namespace calwf

#endif // CALWF_MODEL_MODELPROFILE_H