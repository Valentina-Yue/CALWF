// ============================================================
// CALWF - SafetyFeatures 安全特征
// ============================================================
//
// 文件名：include/calwf/features/SafetyFeatures.h
// 作用：检测文本中的风险信号。
//
// 对应特征（FeatureVector 的 F 类）：
//   risk_word_ratio           风险词占比
//   prompt_residue_ratio      提示残留占比（AI 特有）
//   circular_repetition_ratio 循环重复占比
//
// 说明：
//   - 风险词表从 data/config/risk_words.txt 加载
//   - 循环重复检测用 3-gram：同一 3-gram 出现 3 次以上视为循环
//   - 提示残留检测用滑窗匹配，因为提示句通常是短语
// ============================================================

#ifndef CALWF_FEATURES_SAFETYFEATURES_H
#define CALWF_FEATURES_SAFETYFEATURES_H

#include <string>
#include <vector>

#include "calwf/core/Document.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace features {

class SafetyFeatures {
public:
    // 初始化：加载风险词表
    static bool initialize(const std::string& risk_words_path);

    // 提取
    static void extract(const Document& doc, FeatureVector& fv);

    // 是否已初始化
    static bool isInitialized();

    // 返回命中的风险词列表（用于证据链）
    static std::vector<std::string> findRiskyWords(const Document& doc);
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_SAFETYFEATURES_H