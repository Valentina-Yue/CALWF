// ============================================================
// CALWF - LexicalFeatures 词汇特征
// ============================================================
//
// 文件名：include/calwf/features/LexicalFeatures.h
// 作用：从 Document 中提取词汇丰富度、词长、稀有词等特征。
//
// 对应特征（FeatureVector 的 B 类）：
//   ttr                  Type-Token Ratio
//   sttr                 Standardized TTR（每 1000 词）
//   long_word_ratio      长词（>= 7 字符）占比
//   hapax_ratio          只出现一次的词占比
//   unique_word_ratio    不重复词数 / 总词数
//
// 说明：
//   - 计算时统一转小写，避免 "The" / "the" 被当作两个不同词
//   - TTR 受文本长度影响很大，所以额外提供 STTR
//     STTR = 每 1000 词的 TTR 平均，缓解长度偏差
// ============================================================

#ifndef CALWF_FEATURES_LEXICALFEATURES_H
#define CALWF_FEATURES_LEXICALFEATURES_H

#include "calwf/core/Document.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace features {

class LexicalFeatures {
public:
    // 提取词汇特征并写入 fv
    static void extract(const Document& doc, FeatureVector& fv);

private:
    // 短文本阈值：小于此长度不做 STTR 分段
    static constexpr int STTR_WINDOW = 1000;
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_LEXICALFEATURES_H