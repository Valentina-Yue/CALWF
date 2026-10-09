// ============================================================
// CALWF - DiscourseFeatures 语篇特征
// ============================================================
//
// 文件名：include/calwf/features/DiscourseFeatures.h
// 作用：从 Document 中提取连接词、模板句、代词、重复 n-gram 等
//       语篇层面特征。
//
// 对应特征（FeatureVector 的 D 类）：
//   connective_density      连接词密度
//   template_phrase_ratio   模板短语占比
//   pronoun_ratio           代词占比
//   repetition_ratio        重复 bigram 占比
//
// 说明：
//   - 连接词和模板短语都是多词短语，用滑动窗口匹配。
//   - 重复 n-gram 用 bigram（2 词），统计"出现 >= 2 次的 bigram"
//     占所有 bigram 的比例，用于检测循环和模板化。
//   - 后续会用 Trie / AhoCorasick 替换滑动窗口，提升性能。
// ============================================================

#ifndef CALWF_FEATURES_DISCOURSEFEATURES_H
#define CALWF_FEATURES_DISCOURSEFEATURES_H

#include "calwf/core/Document.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace features {

class DiscourseFeatures {
public:
    // 提取语篇特征并写入 fv
    static void extract(const Document& doc, FeatureVector& fv);
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_DISCOURSEFEATURES_H