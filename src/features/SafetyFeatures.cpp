// ============================================================
// CALWF - SafetyFeatures 实现
// ============================================================

#include "calwf/features/SafetyFeatures.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/Logger.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace calwf {
namespace features {

// 全局风险词表
static std::vector<std::vector<std::string>> g_risk_phrases;
static std::unordered_set<std::string>        g_risk_singles;
static bool                                   g_initialized = false;

bool SafetyFeatures::isInitialized() {
    return g_initialized;
}

// ------------------------------------------------------------
// 初始化
// ------------------------------------------------------------
bool SafetyFeatures::initialize(const std::string& risk_words_path) {
    if (g_initialized) return true;

    auto lines = utils::readLines(risk_words_path);
    if (lines.empty()) {
        CALWF_LOG_WARN("风险词表为空或不存在：%s", risk_words_path.c_str());
        return false;
    }

    g_risk_phrases.clear();
    g_risk_singles.clear();

    for (auto& line : lines) {
        std::string s = utils::trim(line);
        if (s.empty() || s[0] == '#') continue;

        // 转小写
        s = utils::toLower(s);

        // 按空格分词
        auto parts = utils::split(s, ' ');
        if (parts.size() == 1) {
            g_risk_singles.insert(parts[0]);
        } else if (parts.size() > 1) {
            g_risk_phrases.push_back(parts);
        }
    }

    g_initialized = true;
    CALWF_LOG_INFO("风险词表加载完成：%zu 单词, %zu 短语",
                   g_risk_singles.size(),
                   g_risk_phrases.size());
    return true;
}

// ------------------------------------------------------------
// 工具：把文档转成小写词序列（去首尾标点）
// ------------------------------------------------------------
static std::vector<std::string> docToWords(const Document& doc) {
    std::vector<std::string> words;
    words.reserve(static_cast<std::size_t>(doc.word_count));

    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            for (const auto& t : s.tokens) {
                std::string w = utils::toLower(t.word);
                std::size_t start = 0;
                std::size_t end   = w.size();
                while (start < end && !utils::isAlpha(w[start])) ++start;
                while (end > start && !utils::isAlpha(w[end - 1])) --end;
                if (start >= end) continue;
                words.push_back(w.substr(start, end - start));
            }
        }
    }
    return words;
}

// ------------------------------------------------------------
// 匹配短语
// ------------------------------------------------------------
static int matchPhrase(const std::vector<std::string>& words,
                       const std::vector<std::string>& phrase) {
    if (phrase.empty() || words.size() < phrase.size()) return 0;
    int hits = 0;
    const std::size_t N = words.size();
    const std::size_t M = phrase.size();
    for (std::size_t i = 0; i + M <= N; ++i) {
        bool ok = true;
        for (std::size_t j = 0; j < M; ++j) {
            if (words[i + j] != phrase[j]) { ok = false; break; }
        }
        if (ok) ++hits;
    }
    return hits;
}

// ------------------------------------------------------------
// 提取
// ------------------------------------------------------------
void SafetyFeatures::extract(const Document& doc, FeatureVector& fv) {
    if (!g_initialized) return;

    auto words = docToWords(doc);
    const std::size_t N = words.size();
    if (N == 0) return;

    // ---- 1. 风险词 ----
    std::size_t risk_hits = 0;

    // 单词
    for (const auto& w : words) {
        if (g_risk_singles.count(w) > 0) ++risk_hits;
    }

    // 短语
    for (const auto& phrase : g_risk_phrases) {
        risk_hits += static_cast<std::size_t>(matchPhrase(words, phrase));
    }

    fv.risk_word_ratio = static_cast<double>(risk_hits) /
                         static_cast<double>(N);

    // ---- 2. 提示残留 ----
    // 检测 AI 特有的提示残留
    static const std::vector<std::vector<std::string>> residue_phrases = {
        {"as", "an", "ai"},
        {"as", "a", "language", "model"},
        {"as", "an", "assistant"},
        {"i", "cannot", "help"},
        {"i", "am", "sorry"},
        {"i'm", "sorry"},
        {"here", "is", "the", "essay"},
        {"here", "is", "a"},
        {"sure", "here"},
        {"certainly", "here"},
        {"following", "your", "instructions"},
        {"based", "on", "your", "request"},
        {"the", "user", "asked"}
    };

    std::size_t residue_hits = 0;
    for (const auto& phrase : residue_phrases) {
        residue_hits += static_cast<std::size_t>(matchPhrase(words, phrase));
    }
    fv.prompt_residue_ratio = static_cast<double>(residue_hits) /
                              static_cast<double>(N);

    // ---- 3. 循环重复 ----
    // 用 3-gram，统计出现 >= 3 次的 3-gram
    if (N >= 3) {
        std::unordered_map<std::string, int> tri_freq;
        tri_freq.reserve(N);
        for (std::size_t i = 0; i + 3 <= N; ++i) {
            std::string key = words[i] + " " + words[i + 1] + " " + words[i + 2];
            ++tri_freq[key];
        }

        std::size_t total_tris = N - 2;
        std::size_t repeated = 0;
        for (const auto& kv : tri_freq) {
            if (kv.second >= 3) {
                repeated += static_cast<std::size_t>(kv.second - 2);
            }
        }

        fv.circular_repetition_ratio = static_cast<double>(repeated) /
                                       static_cast<double>(total_tris);
    }
}

// ------------------------------------------------------------
// 返回命中的风险词列表（用于证据链）
// ------------------------------------------------------------
std::vector<std::string> SafetyFeatures::findRiskyWords(const Document& doc) {
    std::vector<std::string> hits;
    if (!g_initialized) return hits;

    auto words = docToWords(doc);

    // 单词
    for (const auto& w : words) {
        if (g_risk_singles.count(w) > 0) {
            hits.push_back(w);
        }
    }

    // 短语
    for (const auto& phrase : g_risk_phrases) {
        int n = matchPhrase(words, phrase);
        if (n > 0) {
            std::string joined = utils::join(phrase, " ");
            for (int k = 0; k < n; ++k) {
                hits.push_back(joined);
            }
        }
    }

    return hits;
}

} // namespace features
} // namespace calwf