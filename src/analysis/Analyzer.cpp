// ============================================================
// CALWF - Analyzer 实现
// ============================================================

#include "calwf/analysis/Analyzer.h"
#include "calwf/features/SurfaceFeatures.h"
#include "calwf/features/LexicalFeatures.h"
#include "calwf/features/SyntacticFeatures.h"
#include "calwf/features/DiscourseFeatures.h"
#include "calwf/features/SafetyFeatures.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <map>

namespace calwf {
namespace analysis {

using namespace calwf::features;

// ------------------------------------------------------------
// 设置
// ------------------------------------------------------------
void Analyzer::setModelProfiles(
    const std::vector<model::ModelProfile>& profiles) {
    model_profiles_ = profiles;
}

void Analyzer::setAnchors(const CorpusAnchor* sweccl,
                          const CorpusAnchor* bawe) {
    sweccl_anchor_ = sweccl;
    bawe_anchor_   = bawe;
}

void Analyzer::setReferenceCorpora(const std::vector<Document>* sweccl,
                                   const std::vector<Document>* bawe) {
    sweccl_docs_ = sweccl;
    bawe_docs_   = bawe;
}

// ------------------------------------------------------------
// 提取完整特征向量
// ------------------------------------------------------------
static FeatureVector extractAll(const Document& doc) {
    FeatureVector fv;
    SurfaceFeatures::extract(doc, fv);
    LexicalFeatures::extract(doc, fv);
    SyntacticFeatures::extract(doc, fv);
    DiscourseFeatures::extract(doc, fv);
    SafetyFeatures::extract(doc, fv);
    return fv;
}

// ------------------------------------------------------------
// 加权欧氏距离
// ------------------------------------------------------------
// 关键：每个特征除以该模型组的 stddev（标准化）
// 如果某个模型的 stddev 为 0（某特征完全一致），用全局 fallback
double Analyzer::weightedDistance(
    const FeatureVector& a,
    const FeatureVector& b) const {

    auto va = a.toVector();
    auto vb = b.toVector();
    const std::size_t n = std::min(va.size(), vb.size());

    // 用所有模型 stddev 的均值作为分母
    std::vector<double> sigma(n, 1.0);
    if (!model_profiles_.empty()) {
        for (std::size_t j = 0; j < n; ++j) {
            double s = 0.0;
            int cnt = 0;
            for (const auto& p : model_profiles_) {
                auto vs = p.stddev.toVector();
                if (j < vs.size() && vs[j] > 1e-6) {
                    s += vs[j];
                    ++cnt;
                }
            }
            if (cnt > 0) sigma[j] = s / cnt;
        }
    }

    double sum = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
        double d = (va[j] - vb[j]) / sigma[j];
        sum += d * d;
    }
    return std::sqrt(sum);
}

// ------------------------------------------------------------
// 找最相似的文档
// ------------------------------------------------------------
std::vector<std::string> Analyzer::topSimilarDocs(
    const Document& doc,
    const std::vector<Document>& pool,
    std::size_t top_n) const {

    struct Item {
        std::string doc_id;
        double      score;
    };
    std::vector<Item> scores;
    scores.reserve(pool.size());

    auto v0 = extractAll(doc).toVector();
    const std::size_t n = v0.size();

    for (const auto& d : pool) {
        // 排除自己，避免 SWECCL_A101FYSY 的最近邻是它自己
        if (d.doc_id == doc.doc_id) continue;
        auto v1 = extractAll(d).toVector();
        // 余弦相似度
        double dot = 0, n0 = 0, n1 = 0;
        for (std::size_t j = 0; j < n && j < v1.size(); ++j) {
            dot += v0[j] * v1[j];
            n0  += v0[j] * v0[j];
            n1  += v1[j] * v1[j];
        }
        double cos = (n0 > 0 && n1 > 0) ? dot / (std::sqrt(n0) * std::sqrt(n1)) : 0;
        scores.push_back({d.doc_id, cos});
    }

    std::sort(scores.begin(), scores.end(),
        [](const Item& a, const Item& b) { return a.score > b.score; });

    std::vector<std::string> result;
    std::size_t top_k = std::min(top_n, scores.size());
    for (std::size_t i = 0; i < top_k; ++i) {
        result.push_back(scores[i].doc_id);
    }
    return result;
}

// ------------------------------------------------------------
// 找最异常的句子（简化：找最长的 3 句）
// ------------------------------------------------------------
std::vector<std::string> Analyzer::findAnomalousSentences(
    const Document& doc,
    std::size_t top_n) const {

    struct Item {
        std::string text;
        int         len;
    };
    std::vector<Item> items;

    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            items.push_back({s.text, static_cast<int>(s.tokens.size())});
        }
    }

    std::sort(items.begin(), items.end(),
        [](const Item& a, const Item& b) { return a.len > b.len; });

    std::vector<std::string> result;
    std::size_t top_k = std::min(top_n, items.size());
    for (std::size_t i = 0; i < top_k; ++i) {
        result.push_back(items[i].text);
    }
    return result;
}

// ------------------------------------------------------------
// 主分析函数
// ------------------------------------------------------------
AnalysisResult Analyzer::analyze(const Document& doc) const {
    AnalysisResult r;
    r.doc_id = doc.doc_id;
    r.source = std::string(sourceTypeName(doc.source));

    // ---- 1. 提取完整特征 ----
    FeatureVector fv = extractAll(doc);

    // ---- 2. 锚定分 ----
    if (sweccl_anchor_) {
        fv.sweccl_ngram_coverage = sweccl_anchor_->ngramCoverage(doc, 2);
        fv.sweccl_cosine         = sweccl_anchor_->cosineSimilarity(doc);
    }
    if (bawe_anchor_) {
        fv.bawe_ngram_coverage = bawe_anchor_->ngramCoverage(doc, 2);
        fv.bawe_cosine         = bawe_anchor_->cosineSimilarity(doc);
    }
    if (sweccl_anchor_) {
        fv.oov_rate = sweccl_anchor_->oovRate(doc);
    }

    r.features = fv;
    r.sweccl_anchoring = fv.sweccl_cosine;
    r.bawe_anchoring   = fv.bawe_cosine;

    // ---- 3. 模型归因（kNN） ----
    if (!model_profiles_.empty()) {
        auto knn = knnAttribute(fv, 5);  // k=5

        // 取 top-3
        std::size_t top_k = std::min<std::size_t>(3, knn.size());
        for (std::size_t i = 0; i < top_k; ++i) {
            ModelScore s;
            s.model_name = knn[i].first;
            s.confidence = knn[i].second;
            // 用一个伪距离（与置信度反相关）以保持接口兼容
            s.distance   = 1.0 / (s.confidence + 1e-3);
            r.model_ranking.push_back(s);
        }
    }

    // ---- 4. 四维评分 ----
    //
    // 设计思路：
    //   1. 锚定偏离：1 - max(SWECCL 余弦, BAWE 余弦)
    //      真人文档（无论学生还是母语者）至少和一方接近，
    //      LLM 文档两边都远。
    //   2. 词汇丰富度惩罚：LLM 的 STTR / Hapax / 长词率显著高于真人。
    //      这三项是当前最稳定的 LLM 判别信号。
    //   3. 最终 AI 嫌疑度 = 锚定偏离 + 三个词汇惩罚（截断到 1.0）
    //
    // 实测数据（用于校准）：
    //   SWECCL 学生: sttr≈0.54, hapax≈0.70, longword≈0.23
    //   BAWE  母语者: sttr≈0.41, hapax≈0.65, longword≈0.28
    //   LLM 输出   : sttr≈0.68-0.84, hapax≈0.79-0.88, longword≈0.36-0.49

    double anchor_dev = 1.0 - std::max(fv.sweccl_cosine, fv.bawe_cosine);

    // 词汇丰富度惩罚（分档）
    double vocab_penalty = 0.0;
    if (fv.sttr > 0.70)        vocab_penalty = 0.15;
    else if (fv.sttr > 0.60)   vocab_penalty = 0.08;
    else if (fv.sttr > 0.55)   vocab_penalty = 0.03;

    // Hapax 率惩罚
    double hapax_penalty = 0.0;
    if (fv.hapax_ratio > 0.85)        hapax_penalty = 0.10;
    else if (fv.hapax_ratio > 0.78)   hapax_penalty = 0.05;

    // 长词率惩罚
    double longword_penalty = 0.0;
    if (fv.long_word_ratio > 0.38)        longword_penalty = 0.08;
    else if (fv.long_word_ratio > 0.32)   longword_penalty = 0.04;

    double suspicion = anchor_dev
                     + vocab_penalty
                     + hapax_penalty
                     + longword_penalty;

    r.ai_suspicion   = std::min(1.0, std::max(0.0, suspicion));
    r.human_likeness = 1.0 - r.ai_suspicion;

    // 定性判断
    if (r.ai_suspicion > 0.60)      r.verdict = "AI";
    else if (r.ai_suspicion < 0.35) r.verdict = "Human";
    else                             r.verdict = "Uncertain";

    // ---- 5. 风险等级 ----
    //
    // 修正原则：
    //   - 只有"提示残留"或"极端循环"才是 High
    //   - 学术写作中的 never/always 不算高风险
    //   - 长文本重复率天然高，阈值要放宽
    r.risk_level = "Low";

    if (fv.prompt_residue_ratio > 0.0) {
        r.risk_level = "High";
        r.risk_reasons.push_back("检测到提示残留");
    } else if (fv.circular_repetition_ratio > 0.30) {
        r.risk_level = "High";
        r.risk_reasons.push_back("循环重复极高");
    } else if (fv.risk_word_ratio > 0.02) {
        r.risk_level = "Medium";
        r.risk_reasons.push_back("模板/套话密度偏高");
    } else if (fv.risk_word_ratio > 0.01) {
        r.risk_reasons.push_back("含少量模板词");
    }

    // ---- 6. 证据链 ----
    r.risk_hits = SafetyFeatures::findRiskyWords(doc);

    if (sweccl_docs_ && !sweccl_docs_->empty()) {
        r.top_sweccl_docs = topSimilarDocs(doc, *sweccl_docs_, 3);
    }
    if (bawe_docs_ && !bawe_docs_->empty()) {
        r.top_bawe_docs = topSimilarDocs(doc, *bawe_docs_, 3);
    }

    r.anomalous_sentences = findAnomalousSentences(doc, 3);

    return r;
}

// ------------------------------------------------------------
// kNN 归因
// ------------------------------------------------------------
// 输入：一个查询特征向量
// 输出：按投票数排序的 (model, score) 列表
// 逻辑：
//   1. 把查询向量与所有模型的每个样本计算加权欧氏距离
//   2. 取最近的 K 个样本
//   3. 统计这 K 个样本的模型分布
//   4. 投票数归一化为 0-1 的得分
// ------------------------------------------------------------
std::vector<std::pair<std::string, double>>
Analyzer::knnAttribute(const features::FeatureVector& fv,
                       std::size_t k) const {

    struct Neighbor {
        std::string model;
        double      distance;
    };
    std::vector<Neighbor> all;

    for (const auto& p : model_profiles_) {
        for (const auto& s : p.samples) {
            double d = weightedDistance(fv, s);
            all.push_back({p.model_name, d});
        }
    }

    if (all.empty()) return {};

    // 按距离排序
    std::sort(all.begin(), all.end(),
        [](const Neighbor& a, const Neighbor& b) {
            return a.distance < b.distance;
        });

    // 取最近的 k 个
    std::size_t top_k = std::min(k, all.size());

    // 统计
    std::map<std::string, double> votes;
    for (std::size_t i = 0; i < top_k; ++i) {
        // 距离越近权重越大（1/(d+eps) 加权投票）
        double w = 1.0 / (all[i].distance + 1e-3);
        votes[all[i].model] += w;
    }

    // 归一化
    std::vector<std::pair<std::string, double>> result;
    double total = 0.0;
    for (const auto& kv : votes) {
        total += kv.second;
    }
    for (const auto& kv : votes) {
        result.push_back({kv.first, kv.second / total});
    }

    // 按得分排序
    std::sort(result.begin(), result.end(),
        [](const std::pair<std::string, double>& a,
           const std::pair<std::string, double>& b) {
            return a.second > b.second;
        });

    return result;
}

} // namespace analysis
} // namespace calwf