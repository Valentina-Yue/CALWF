// ============================================================
// CALWF - DiscourseFeatures 实现
// ============================================================

#include "calwf/features/DiscourseFeatures.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace calwf {
namespace features {

// ============================================================
// 词表
// ============================================================

// ------------------------------------------------------------
// 连接词表（单词）
// ------------------------------------------------------------
static const std::unordered_set<std::string>& connectivesSingle() {
    static const std::unordered_set<std::string> s = {
        "however", "therefore", "moreover", "furthermore",
        "nevertheless", "nonetheless", "consequently",
        "meanwhile", "otherwise", "thus", "hence",
        "also", "besides", "finally", "firstly", "secondly",
        "thirdly", "lastly", "similarly", "conversely",
        "additionally"
    };
    return s;
}

// ------------------------------------------------------------
// 连接词表（多词短语）
// ------------------------------------------------------------
static const std::vector<std::vector<std::string>>& connectivesMulti() {
    static const std::vector<std::vector<std::string>> s = {
        {"in", "addition"},
        {"for", "example"},
        {"for", "instance"},
        {"in", "conclusion"},
        {"in", "fact"},
        {"in", "other", "words"},
        {"on", "the", "other", "hand"},
        {"on", "the", "contrary"},
        {"as", "a", "result"},
        {"as", "a", "consequence"},
        {"in", "contrast"},
        {"by", "contrast"},
        {"in", "summary"},
        {"to", "sum", "up"},
        {"first", "of", "all"},
        {"above", "all"},
        {"after", "all"},
        {"all", "in", "all"},
        {"in", "short"},
        {"in", "brief"},
        {"as", "well", "as"},
        {"not", "only", "but", "also"},
        {"more", "and", "more"},
        {"last", "but", "not", "least"}
    };
    return s;
}

// ------------------------------------------------------------
// 模板短语表（用于检测 AI 套话）
// ------------------------------------------------------------
static const std::vector<std::vector<std::string>>& templatePhrases() {
    static const std::vector<std::vector<std::string>> s = {
        {"with", "the", "development", "of"},
        {"with", "the", "rapid", "development", "of"},
        {"with", "the", "advancement", "of"},
        {"it", "is", "widely", "believed", "that"},
        {"it", "is", "commonly", "believed", "that"},
        {"it", "is", "generally", "acknowledged", "that"},
        {"it", "is", "worth", "noting", "that"},
        {"it", "is", "important", "to", "note", "that"},
        {"it", "goes", "without", "saying", "that"},
        {"there", "is", "no", "doubt", "that"},
        {"in", "today's", "world"},
        {"in", "the", "modern", "world"},
        {"in", "this", "day", "and", "age"},
        {"plays", "an", "important", "role", "in"},
        {"plays", "a", "crucial", "role", "in"},
        {"plays", "a", "vital", "role", "in"},
        {"on", "the", "one", "hand"},
        {"as", "we", "all", "know"},
        {"from", "my", "perspective"},
        {"in", "my", "opinion"},
        {"take", "measures", "to"},
        {"attach", "great", "importance", "to"},
        {"shed", "light", "on"},
        {"last", "but", "not", "least"},
        {"when", "it", "comes", "to"},
        {"a", "double", "edged", "sword"},
        {"a", "double-edged", "sword"}
    };
    return s;
}

// ------------------------------------------------------------
// 代词表
// ------------------------------------------------------------
static const std::unordered_set<std::string>& pronouns() {
    static const std::unordered_set<std::string> s = {
        "i", "you", "he", "she", "it", "we", "they",
        "me", "him", "her", "us", "them",
        "my", "your", "his", "its", "our", "their",
        "mine", "yours", "hers", "ours", "theirs",
        "myself", "yourself", "himself", "herself",
        "itself", "ourselves", "yourselves", "themselves",
        "this", "that", "these", "those"
    };
    return s;
}

// ============================================================
// 辅助函数
// ============================================================

// ------------------------------------------------------------
// 收集文档的词（小写，已过滤标点）
// ------------------------------------------------------------
// 关键：去掉 token 首尾的标点，避免 "conclusion," 匹配不上 "conclusion"
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
                while (start < end && !utils::isAlpha(t.word[start])) {
                    ++start;
                }
                while (end > start && !utils::isAlpha(t.word[end - 1])) {
                    --end;
                }
                if (start >= end) continue;

                std::string clean = t.word.substr(start, end - start);
                words.push_back(utils::toLower(clean));
            }
        }
    }
    return words;
}

// ------------------------------------------------------------
// 在词序列中匹配一个短语（返回命中次数）
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
            if (words[i + j] != phrase[j]) {
                ok = false;
                break;
            }
        }
        if (ok) {
            ++hits;
        }
    }
    return hits;
}

// ============================================================
// 主提取函数
// ============================================================
void DiscourseFeatures::extract(const Document& doc, FeatureVector& fv) {
    auto words = collectWordsLower(doc);
    const std::size_t N = words.size();
    if (N == 0) return;

    // ---------- 1. 连接词 ----------
    std::size_t conn_hits = 0;

    // 单词连接词
    for (const auto& w : words) {
        if (connectivesSingle().count(w) > 0) {
            ++conn_hits;
        }
    }

    // 多词连接词
    for (const auto& phrase : connectivesMulti()) {
        conn_hits += static_cast<std::size_t>(matchPhrase(words, phrase));
    }

    fv.connective_density = static_cast<double>(conn_hits) /
                            static_cast<double>(N);

    // ---------- 2. 模板短语 ----------
    std::size_t tpl_hits = 0;
    for (const auto& phrase : templatePhrases()) {
        tpl_hits += static_cast<std::size_t>(matchPhrase(words, phrase));
    }

    fv.template_phrase_ratio = static_cast<double>(tpl_hits) /
                               static_cast<double>(N);

    // ---------- 3. 代词 ----------
    std::size_t pron_hits = 0;
    for (const auto& w : words) {
        if (pronouns().count(w) > 0) ++pron_hits;
    }
    fv.pronoun_ratio = static_cast<double>(pron_hits) /
                       static_cast<double>(N);

    // ---------- 4. 重复 bigram ----------
    // 统计所有 bigram 的频次，找出"出现 >= 2 次"的 bigram 数量
    if (N >= 2) {
        std::unordered_map<std::string, int> bigram_freq;
        bigram_freq.reserve(N);
        for (std::size_t i = 0; i + 2 <= N; ++i) {
            std::string key = words[i] + " " + words[i + 1];
            ++bigram_freq[key];
        }

        std::size_t total_bigrams = N - 1;
        std::size_t repeated_bigrams = 0;
        for (const auto& kv : bigram_freq) {
            if (kv.second >= 2) {
                repeated_bigrams += static_cast<std::size_t>(kv.second - 1);
            }
        }

        fv.repetition_ratio = static_cast<double>(repeated_bigrams) /
                              static_cast<double>(total_bigrams);
    }
}

} // namespace features
} // namespace calwf