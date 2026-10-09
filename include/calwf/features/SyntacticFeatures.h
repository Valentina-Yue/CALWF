// ============================================================
// CALWF - SyntacticFeatures 句法特征
// ============================================================
//
// 文件名：include/calwf/features/SyntacticFeatures.h
// 作用：从 Document 中提取句法相关特征（简化版，基于词形启发式）。
//
// 对应特征（FeatureVector 的 C 类）：
//   passive_ratio          被动语态估计
//   clause_marker_ratio    从句标记词占比
//   modal_ratio            情态动词占比
//   nominalization_ratio   名词化后缀占比
//   gerund_ratio           -ing 形式占比
//
// 说明：
//   - 我们不用完整的 POS tagger，因为 SWECCL 和 LLM 没有 POS。
//   - 这里用词形规则 + 小词表做近似估计。
//   - 对 BAWE（有 POS）来说，这些估计已经足够验证。
//   - 后续如果精度不够，可以替换为基于 POS 的精确版本。
// ============================================================

#ifndef CALWF_FEATURES_SYNTACTICFEATURES_H
#define CALWF_FEATURES_SYNTACTICFEATURES_H

#include "calwf/core/Document.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace features {

class SyntacticFeatures {
public:
    // 提取句法特征并写入 fv
    static void extract(const Document& doc, FeatureVector& fv);

    // 单句级别估计（用于证据句识别）
    static bool isPassiveSentence(const Sentence& s);
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_SYNTACTICFEATURES_H