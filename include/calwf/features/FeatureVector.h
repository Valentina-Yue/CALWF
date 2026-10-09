// ============================================================
// CALWF - FeatureVector 特征向量
// ============================================================
//
// 文件名：include/calwf/features/FeatureVector.h
// 作用：统一承载一个文档的所有数值特征，供后续分类、聚类、
//       相似度比较使用。
//
// 特征分 6 大类：
//   A. 表层特征（Surface）   句子长度、词长、标点
//   B. 词汇特征（Lexical）   TTR、长词、稀有词
//   C. 句法特征（Syntactic） 被动、从句、情态、名词化
//   D. 语篇特征（Discourse） 连接词、模板句、代词
//   E. 锚定特征（Anchoring） SWECCL/BAWE n-gram 命中率
//   F. 安全特征（Safety）    风险词、提示残留、循环重复
//
// 说明：
//   - 所有特征都是 double，方便直接做欧氏距离、余弦相似度
//   - 也提供 toVector() / fromVector() 用于序列化和聚类
//   - featureNames() 返回特征名列表，用于可解释报告
// ============================================================

#ifndef CALWF_FEATURES_FEATUREVECTOR_H
#define CALWF_FEATURES_FEATUREVECTOR_H

#include <string>
#include <vector>

namespace calwf {
namespace features {

// ------------------------------------------------------------
// FeatureVector
// ------------------------------------------------------------
struct FeatureVector {
    // ============ A. 表层特征 ============
    double word_count              = 0.0;  // 总词数
    double sentence_count          = 0.0;  // 句子数
    double paragraph_count         = 0.0;  // 段落数
    double avg_sentence_length     = 0.0;  // 平均句长（词）
    double std_sentence_length     = 0.0;  // 句长标准差
    double max_sentence_length     = 0.0;  // 最长句长
    double min_sentence_length     = 0.0;  // 最短句长
    double avg_word_length         = 0.0;  // 平均词长（字符）
    double punctuation_density     = 0.0;  // 标点数 / 词数
    double uppercase_ratio         = 0.0;  // 大写字母 / 全部字母

    // ============ B. 词汇特征 ============
    double ttr                     = 0.0;  // Type-Token Ratio
    double sttr                    = 0.0;  // Standardized TTR（每 1000 词）
    double long_word_ratio         = 0.0;  // 长度 >= 7 的词占比
    double hapax_ratio             = 0.0;  // 只出现一次的词占比
    double unique_word_ratio       = 0.0;  // 不重复词数 / 总词数

    // ============ C. 句法特征（简化版） ============
    double passive_ratio           = 0.0;  // 被动语态估计
    double clause_marker_ratio     = 0.0;  // 从句标记词占比
    double modal_ratio             = 0.0;  // 情态动词占比
    double nominalization_ratio    = 0.0;  // 名词化后缀占比
    double gerund_ratio            = 0.0;  // -ing 形式占比

    // ============ D. 语篇特征 ============
    double connective_density      = 0.0;  // 连接词密度
    double template_phrase_ratio   = 0.0;  // 模板短语占比
    double pronoun_ratio           = 0.0;  // 代词占比
    double repetition_ratio        = 0.0;  // 重复 n-gram 占比

    // ============ E. 锚定特征（待语料库就绪） ============
    double sweccl_ngram_coverage   = 0.0;
    double bawe_ngram_coverage     = 0.0;
    double oov_rate                = 0.0;
    double sweccl_cosine           = 0.0;
    double bawe_cosine             = 0.0;

    // ============ F. 安全特征（待词表就绪） ============
    double risk_word_ratio         = 0.0;
    double prompt_residue_ratio    = 0.0;
    double circular_repetition_ratio = 0.0;

    // ============ 序列化 ============

    // 转成向量（用于聚类、距离计算）
    std::vector<double> toVector() const;

    // 从向量构造（用于反序列化）
    static FeatureVector fromVector(const std::vector<double>& v);

    // 特征名列表（顺序与 toVector 一致）
    static std::vector<std::string> featureNames();

    // 特征总数
    static std::size_t featureCount();

    // 调试打印
    void print() const;
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_FEATUREVECTOR_H