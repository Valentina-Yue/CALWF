// ============================================================
// CALWF - SurfaceFeatures 实现
// ============================================================

#include "calwf/features/SurfaceFeatures.h"
#include "calwf/utils/StringUtils.h"

#include <cmath>
#include <vector>

namespace calwf {
namespace features {

// ------------------------------------------------------------
// 从一个句子统计词数
// ------------------------------------------------------------
static int sentenceWordCount(const Sentence& s) {
    if (!s.tokens.empty()) {
        return static_cast<int>(s.tokens.size());
    }
    // 没有 token，按空格粗略统计
    return s.wordCount();
}

// ------------------------------------------------------------
// 统计整篇文档的表层特征
// ------------------------------------------------------------
void SurfaceFeatures::extract(const Document& doc, FeatureVector& fv) {
    // ---- 基础计数 ----
    fv.word_count      = static_cast<double>(doc.word_count);
    fv.sentence_count  = static_cast<double>(doc.sentence_count);
    fv.paragraph_count = static_cast<double>(doc.paragraph_count);

    // ---- 句子长度统计 ----
    std::vector<double> sent_lengths;
    sent_lengths.reserve(
        static_cast<std::size_t>(doc.sentence_count)
    );

    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            double len = static_cast<double>(sentenceWordCount(s));
            sent_lengths.push_back(len);
        }
    }

    if (!sent_lengths.empty()) {
        // 平均
        double sum = 0;
        for (double d : sent_lengths) sum += d;
        double mean = sum / static_cast<double>(sent_lengths.size());
        fv.avg_sentence_length = mean;

        // 标准差
        double sq_sum = 0;
        for (double d : sent_lengths) {
            sq_sum += (d - mean) * (d - mean);
        }
        fv.std_sentence_length =
            std::sqrt(sq_sum / static_cast<double>(sent_lengths.size()));

        // max / min
        double mx = sent_lengths[0];
        double mn = sent_lengths[0];
        for (double d : sent_lengths) {
            if (d > mx) mx = d;
            if (d < mn) mn = d;
        }
        fv.max_sentence_length = mx;
        fv.min_sentence_length = mn;
    }

    // ---- 词长 & 标点 & 大小写 ----
    // 收集所有词
    long total_chars = 0;
    long word_count_for_chars = 0;
    long punct_count = 0;
    long upper_count = 0;
    long alpha_count = 0;

    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            for (const auto& t : s.tokens) {
                if (t.word.empty()) continue;
                total_chars += static_cast<long>(t.word.size());
                ++word_count_for_chars;

                for (char c : t.word) {
                    if (utils::isPunctuation(c)) ++punct_count;
                    if (c >= 'A' && c <= 'Z') ++upper_count;
                    if (utils::isAlpha(c))    ++alpha_count;
                }
            }
        }
    }

    // 如果 tokens 为空（有些文档可能没有 token），用 raw_text 统计
    if (word_count_for_chars == 0) {
        for (char c : doc.raw_text) {
            if (utils::isAlpha(c)) {
                ++alpha_count;
                if (c >= 'A' && c <= 'Z') ++upper_count;
            }
            if (utils::isPunctuation(c)) ++punct_count;
        }
        total_chars = static_cast<long>(doc.raw_text.size());
        word_count_for_chars = std::max(1L, static_cast<long>(doc.word_count));
    }

    // 平均词长（用实际字符数 / 词数）
    if (word_count_for_chars > 0) {
        fv.avg_word_length =
            static_cast<double>(total_chars) /
            static_cast<double>(word_count_for_chars);
    }

    // 标点密度
    if (word_count_for_chars > 0) {
        fv.punctuation_density =
            static_cast<double>(punct_count) /
            static_cast<double>(word_count_for_chars);
    }

    // 大写比例
    if (alpha_count > 0) {
        fv.uppercase_ratio =
            static_cast<double>(upper_count) /
            static_cast<double>(alpha_count);
    }
}

} // namespace features
} // namespace calwf