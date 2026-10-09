// ============================================================
// CALWF - SurfaceFeatures 表层特征
// ============================================================
//
// 文件名：include/calwf/features/SurfaceFeatures.h
// 作用：从 Document 中提取表层特征（句子长度、词长、标点、
//       大小写等）。
//
// 对应特征（FeatureVector 的 A 类）：
//   word_count / sentence_count / paragraph_count
//   avg_sentence_length / std_sentence_length
//   max_sentence_length / min_sentence_length
//   avg_word_length / punctuation_density / uppercase_ratio
// ============================================================

#ifndef CALWF_FEATURES_SURFACEFEATURES_H
#define CALWF_FEATURES_SURFACEFEATURES_H

#include "calwf/core/Document.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace features {

class SurfaceFeatures {
public:
    // 从 Document 提取表层特征并写入 fv
    static void extract(const Document& doc, FeatureVector& fv);
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_SURFACEFEATURES_H