// ============================================================
// CALWF - LexicalFeatures 实现
// ============================================================

#include "calwf/features/LexicalFeatures.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace calwf {
namespace features {

// ------------------------------------------------------------
// 收集文档中的词（小写形式，已去首尾标点）
// ------------------------------------------------------------
static std::vector<std::string> collectWordsLower(const Document& doc) {
    std::vector<std::string> words;
    words.reserve(static_cast<std::size_t>(doc.word_count));

    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            for (const auto& t : s.tokens) {
                if (t.word.empty()) continue;

                // 去掉首尾非字母字符
                std::size_t start = 0;
                std::size_t end   = t.word.size();
                while (start < end && !utils::isAlpha(t.word[start])) ++start;
                while (end > start && !utils::isAlpha(t.word[end - 1])) --end;
                if (start >= end) continue;

                std::string clean = t.word.substr(start, end - start);
                words.push_back(utils::toLower(clean));
            }
        }
    }

    // 如果 tokens 为空，从 raw_text 粗暴分词
    if (words.empty() && !doc.raw_text.empty()) {
        std::string word;
        for (char c : doc.raw_text) {
            if (utils::isAlpha(c) || c == '\'') {
                word.push_back(c);
            } else {
                if (!word.empty()) {
                    words.push_back(utils::toLower(word));
                    word.clear();
                }
            }
        }
        if (!word.empty()) words.push_back(utils::toLower(word));
    }

    return words;
}

// ------------------------------------------------------------
// 提取
// ------------------------------------------------------------
void LexicalFeatures::extract(const Document& doc, FeatureVector& fv) {
    auto words = collectWordsLower(doc);

    const std::size_t N = words.size();
    if (N == 0) {
        return;  // 无词，特征保持 0
    }

    // ---- 词频统计 ----
    std::unordered_map<std::string, int> freq;
    freq.reserve(N);
    for (const auto& w : words) {
        ++freq[w];
    }

    const std::size_t V = freq.size();    // 不同词数

    // ---- TTR ----
    fv.ttr = static_cast<double>(V) / static_cast<double>(N);
    fv.unique_word_ratio = fv.ttr;  // 暂时等价；后续可以分开定义

    // ---- STTR（每 1000 词的 TTR 平均） ----
    if (N <= static_cast<std::size_t>(STTR_WINDOW)) {
        // 短文本，直接 TTR
        fv.sttr = fv.ttr;
    } else {
        // 按 1000 词切片，每片算 TTR，再平均
        double ttr_sum = 0.0;
        int    chunks  = 0;
        for (std::size_t start = 0;
             start + static_cast<std::size_t>(STTR_WINDOW) <= N;
             start += static_cast<std::size_t>(STTR_WINDOW)) {
            std::unordered_set<std::string> seen;
            for (std::size_t i = start;
                 i < start + static_cast<std::size_t>(STTR_WINDOW); ++i) {
                seen.insert(words[i]);
            }
            double chunk_ttr = static_cast<double>(seen.size()) /
                               static_cast<double>(STTR_WINDOW);
            ttr_sum += chunk_ttr;
            ++chunks;
        }
        fv.sttr = (chunks > 0) ? (ttr_sum / chunks) : fv.ttr;
    }

    // ---- 长词比例（>= 7 字符） ----
    std::size_t long_cnt = 0;
    for (const auto& w : words) {
        if (w.size() >= 7) ++long_cnt;
    }
    fv.long_word_ratio = static_cast<double>(long_cnt) /
                         static_cast<double>(N);

    // ---- Hapax ratio（只出现一次的词） ----
    std::size_t hapax = 0;
    for (const auto& kv : freq) {
        if (kv.second == 1) ++hapax;
    }
    fv.hapax_ratio = static_cast<double>(hapax) /
                     static_cast<double>(V);
}

} // namespace features
} // namespace calwf