// ============================================================
// CALWF - FeatureVector 实现
// ============================================================

#include "calwf/features/FeatureVector.h"

#include <iostream>
#include <iomanip>

namespace calwf {
namespace features {

// ------------------------------------------------------------
// 转成向量
// ------------------------------------------------------------
std::vector<double> FeatureVector::toVector() const {
    return {
        // A. 表层
        word_count,
        sentence_count,
        paragraph_count,
        avg_sentence_length,
        std_sentence_length,
        max_sentence_length,
        min_sentence_length,
        avg_word_length,
        punctuation_density,
        uppercase_ratio,

        // B. 词汇
        ttr,
        sttr,
        long_word_ratio,
        hapax_ratio,
        unique_word_ratio,

        // C. 句法
        passive_ratio,
        clause_marker_ratio,
        modal_ratio,
        nominalization_ratio,
        gerund_ratio,

        // D. 语篇
        connective_density,
        template_phrase_ratio,
        pronoun_ratio,
        repetition_ratio,

        // E. 锚定
        sweccl_ngram_coverage,
        bawe_ngram_coverage,
        oov_rate,
        sweccl_cosine,
        bawe_cosine,

        // F. 安全
        risk_word_ratio,
        prompt_residue_ratio,
        circular_repetition_ratio
    };
}

// ------------------------------------------------------------
// 从向量构造
// ------------------------------------------------------------
FeatureVector FeatureVector::fromVector(const std::vector<double>& v) {
    FeatureVector fv;
    std::size_t i = 0;
    auto safe = [&](double d) -> double {
        return (i < v.size()) ? v[i++] : d;
    };

    fv.word_count              = safe(0);
    fv.sentence_count          = safe(0);
    fv.paragraph_count         = safe(0);
    fv.avg_sentence_length     = safe(0);
    fv.std_sentence_length     = safe(0);
    fv.max_sentence_length     = safe(0);
    fv.min_sentence_length     = safe(0);
    fv.avg_word_length         = safe(0);
    fv.punctuation_density     = safe(0);
    fv.uppercase_ratio         = safe(0);

    fv.ttr                     = safe(0);
    fv.sttr                    = safe(0);
    fv.long_word_ratio         = safe(0);
    fv.hapax_ratio             = safe(0);
    fv.unique_word_ratio       = safe(0);

    fv.passive_ratio           = safe(0);
    fv.clause_marker_ratio     = safe(0);
    fv.modal_ratio             = safe(0);
    fv.nominalization_ratio    = safe(0);
    fv.gerund_ratio            = safe(0);

    fv.connective_density      = safe(0);
    fv.template_phrase_ratio   = safe(0);
    fv.pronoun_ratio           = safe(0);
    fv.repetition_ratio        = safe(0);

    fv.sweccl_ngram_coverage   = safe(0);
    fv.bawe_ngram_coverage     = safe(0);
    fv.oov_rate                = safe(0);
    fv.sweccl_cosine           = safe(0);
    fv.bawe_cosine             = safe(0);

    fv.risk_word_ratio         = safe(0);
    fv.prompt_residue_ratio    = safe(0);
    fv.circular_repetition_ratio = safe(0);

    return fv;
}

// ------------------------------------------------------------
// 特征名列表
// ------------------------------------------------------------
std::vector<std::string> FeatureVector::featureNames() {
    return {
        "word_count",
        "sentence_count",
        "paragraph_count",
        "avg_sentence_length",
        "std_sentence_length",
        "max_sentence_length",
        "min_sentence_length",
        "avg_word_length",
        "punctuation_density",
        "uppercase_ratio",

        "ttr",
        "sttr",
        "long_word_ratio",
        "hapax_ratio",
        "unique_word_ratio",

        "passive_ratio",
        "clause_marker_ratio",
        "modal_ratio",
        "nominalization_ratio",
        "gerund_ratio",

        "connective_density",
        "template_phrase_ratio",
        "pronoun_ratio",
        "repetition_ratio",

        "sweccl_ngram_coverage",
        "bawe_ngram_coverage",
        "oov_rate",
        "sweccl_cosine",
        "bawe_cosine",

        "risk_word_ratio",
        "prompt_residue_ratio",
        "circular_repetition_ratio"
    };
}

std::size_t FeatureVector::featureCount() {
    return featureNames().size();
}

// ------------------------------------------------------------
// 调试打印
// ------------------------------------------------------------
void FeatureVector::print() const {
    auto names = featureNames();
    auto vals  = toVector();

    for (std::size_t i = 0; i < names.size() && i < vals.size(); ++i) {
        std::cout << "  " << std::setw(30) << std::left << names[i]
                  << " = " << std::fixed << std::setprecision(4)
                  << vals[i] << "\n";
    }
}

} // namespace features
} // namespace calwf