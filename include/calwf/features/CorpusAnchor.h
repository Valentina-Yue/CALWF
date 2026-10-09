// ============================================================
// CALWF - CorpusAnchor 语料库锚定器
// ============================================================
//
// 文件名：include/calwf/features/CorpusAnchor.h
// 作用：从一个语料库（SWECCL 或 BAWE）构建 n-gram 频率表，
//       并支持：
//         - bigram 覆盖率（文档的 bigram 有多少在语料库中出现）
//         - unigram 覆盖率
//         - OOV 率（文档的词有多少不在语料库词表中）
//         - 余弦相似度（文档词频向量 vs 语料库词频向量）
//
// 缓存：
//   构建一次后可以保存为二进制文件，下次直接加载。
//   路径由调用方指定，例如：
//     D:/CALWF/data/index/sweccl.ngram
//     D:/CALWF/data/index/bawe.ngram
//
// 内存估算：
//   SWECCL  ~100 万词，unigram ~3 万，bigram ~50 万
//   BAWE    ~550 万词，unigram ~8 万，bigram ~200 万
//   总体约 1-2 GB。可接受。
//
// 对应数据结构课程知识：
//   - 哈希表（unordered_map 存储 n-gram 频次）
//   - 串（拼接 bigram key）
//   - 排序（可选：按频次排序导出）
// ============================================================

#ifndef CALWF_FEATURES_CORPUSANCHOR_H
#define CALWF_FEATURES_CORPUSANCHOR_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "calwf/core/Corpus.h"
#include "calwf/core/Document.h"

namespace calwf {
namespace features {

class CorpusAnchor {
public:
    CorpusAnchor() = default;

    // ---- 构建 ----
    // 从语料库构建 n-gram 表
    void build(const Corpus& corpus);

    // ---- 保存 / 加载 ----
    bool save(const std::string& path) const;
    bool load(const std::string& path);

    // ---- 覆盖率 ----
    // 文档的 n-gram 中，有多少在语料库中出现过
    // n = 1, 2, 3
    double ngramCoverage(const Document& doc, int n) const;

    // ---- OOV 率 ----
    double oovRate(const Document& doc) const;

    // ---- 余弦相似度 ----
    // 基于 unigram 词频分布
    double cosineSimilarity(const Document& doc) const;

    // ---- 统计 ----
    std::size_t vocabSize() const { return unigram_.size(); }
    std::size_t bigramSize() const { return bigram_.size(); }
    std::size_t trigramSize() const { return trigram_.size(); }
    std::uint64_t totalTokens() const { return total_tokens_; }

private:
    std::unordered_map<std::string, std::uint32_t> unigram_;
    std::unordered_map<std::string, std::uint32_t> bigram_;
    std::unordered_map<std::string, std::uint32_t> trigram_;
    std::uint64_t total_tokens_ = 0;

    // ---- 工具函数 ----
    // 把文档转成词序列（小写，去首尾标点）
    static std::vector<std::string> docToWords(const Document& doc);

    // 从词序列生成 bigram key
    static std::string makeBigramKey(const std::string& a,
                                     const std::string& b);

    // 从词序列生成 trigram key
    static std::string makeTrigramKey(const std::string& a,
                                      const std::string& b,
                                      const std::string& c);
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_CORPUSANCHOR_H