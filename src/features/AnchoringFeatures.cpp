// ============================================================
// CALWF - AnchoringFeatures 实现
// ============================================================

#include "calwf/features/AnchoringFeatures.h"
#include "calwf/io/SwecclImporter.h"
#include "calwf/io/BaweImporter.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/Logger.h"
#include <iostream>

namespace calwf {
namespace features {

// 全局语料库锚定器
static CorpusAnchor g_sweccl_anchor;
static CorpusAnchor g_bawe_anchor;
static bool         g_initialized = false;

bool AnchoringFeatures::isInitialized() {
    return g_initialized;
}

// ------------------------------------------------------------
// 初始化
// ------------------------------------------------------------
void AnchoringFeatures::initialize(
    const std::string& sweccl_corpus_dir,
    const std::string& bawe_meta_path,
    const std::string& bawe_csv_dir,
    const std::string& sweccl_cache,
    const std::string& bawe_cache
) {
    if (g_initialized) return;

    // ---- SWECCL ----
    if (!g_sweccl_anchor.load(sweccl_cache)) {
        CALWF_LOG_INFO("SWECCL 缓存不存在，开始构建...");
        io::SwecclImporter sw;
        auto sw_corpus_docs = sw.load(sweccl_corpus_dir);
        Corpus sw_corpus;
        for (auto& d : sw_corpus_docs) {
            sw_corpus.add(std::move(d));
        }
        g_sweccl_anchor.build(sw_corpus);
        g_sweccl_anchor.save(sweccl_cache);
    }

    // ---- BAWE ----
    if (!g_bawe_anchor.load(bawe_cache)) {
        CALWF_LOG_INFO("BAWE 缓存不存在，开始构建...");
        io::BaweImporter bw;
        auto bw_corpus_docs = bw.load(bawe_meta_path, bawe_csv_dir);
        Corpus bw_corpus;
        for (auto& d : bw_corpus_docs) {
            bw_corpus.add(std::move(d));
        }
        g_bawe_anchor.build(bw_corpus);
        g_bawe_anchor.save(bawe_cache);
    }

    g_initialized = true;
}

// ------------------------------------------------------------
// 提取特征
// ------------------------------------------------------------
void AnchoringFeatures::extract(const Document& doc, FeatureVector& fv) {
    if (!g_initialized) return;

    // 用 bigram 覆盖率（比 unigram 更有区分度）
    fv.sweccl_ngram_coverage = g_sweccl_anchor.ngramCoverage(doc, 2);
    fv.bawe_ngram_coverage   = g_bawe_anchor.ngramCoverage(doc, 2);

    // OOV 用 SWECCL 词表
    fv.oov_rate = g_sweccl_anchor.oovRate(doc);

    // 余弦相似度
    fv.sweccl_cosine = g_sweccl_anchor.cosineSimilarity(doc);
    fv.bawe_cosine   = g_bawe_anchor.cosineSimilarity(doc);
}

// ------------------------------------------------------------
// 调试打印
// ------------------------------------------------------------
void AnchoringFeatures::printStats() {
    if (!g_initialized) {
        std::cout << "  AnchoringFeatures 未初始化\n";
        return;
    }
    std::cout << "  SWECCL 规模："
              << g_sweccl_anchor.vocabSize() << " unigram, "
              << g_sweccl_anchor.bigramSize() << " bigram, "
              << g_sweccl_anchor.trigramSize() << " trigram, "
              << g_sweccl_anchor.totalTokens() << " tokens\n";
    std::cout << "  BAWE 规模："
              << g_bawe_anchor.vocabSize() << " unigram, "
              << g_bawe_anchor.bigramSize() << " bigram, "
              << g_bawe_anchor.trigramSize() << " trigram, "
              << g_bawe_anchor.totalTokens() << " tokens\n";
}

// ------------------------------------------------------------
// 暴露内部 anchor（避免重复加载）
// ------------------------------------------------------------
const CorpusAnchor& AnchoringFeatures::swecclAnchor() {
    return g_sweccl_anchor;
}

const CorpusAnchor& AnchoringFeatures::baweAnchor() {
    return g_bawe_anchor;
}

} // namespace features
} // namespace calwf