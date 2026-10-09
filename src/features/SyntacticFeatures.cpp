// ============================================================
// CALWF - SyntacticFeatures 实现
// ============================================================

#include "calwf/features/SyntacticFeatures.h"
#include "calwf/utils/StringUtils.h"

#include <unordered_set>
#include <vector>

namespace calwf {
namespace features {

// ------------------------------------------------------------
// 情态动词表
// ------------------------------------------------------------
static const std::unordered_set<std::string>& modalVerbs() {
    static const std::unordered_set<std::string> s = {
        "can", "could", "may", "might", "must",
        "shall", "should", "will", "would",
        "ought", "need", "dare"
    };
    return s;
}

// ------------------------------------------------------------
// Be 动词表（用于被动估计）
// ------------------------------------------------------------
static const std::unordered_set<std::string>& beVerbs() {
    static const std::unordered_set<std::string> s = {
        "is", "are", "was", "were", "be", "been", "being",
        "am", "isn't", "aren't", "wasn't", "weren't"
    };
    return s;
}

// ------------------------------------------------------------
// 从句标记词表
// ------------------------------------------------------------
static const std::unordered_set<std::string>& clauseMarkers() {
    static const std::unordered_set<std::string> s = {
        // 关系代词
        "that", "which", "who", "whom", "whose", "where", "when", "why",
        // 从属连词
        "because", "although", "though", "if", "unless", "until",
        "while", "whereas", "since", "as", "whether",
        "after", "before", "once", "whenever", "wherever", "however",
        // 名词性从句引导
        "what", "whatever", "whichever", "whoever"
    };
    return s;
}

// ------------------------------------------------------------
// 名词化后缀
// ------------------------------------------------------------
static const std::vector<std::string>& nominalizationSuffixes() {
    static const std::vector<std::string> s = {
        "tion", "sion", "ment", "ness", "ity", "ance", "ence",
        "ism", "ship", "hood", "age", "ure"
    };
    return s;
}

// ------------------------------------------------------------
// 判断一个词是否为"过去分词形式"（启发式）
// ------------------------------------------------------------
static bool looksLikePastParticiple(const std::string& word_lower) {
    if (word_lower.size() < 4) return false;
    // 规则动词：-ed 结尾
    if (utils::endsWith(word_lower, "ed")) return true;
    // 不规则动词（常见表）
    static const std::unordered_set<std::string> irregular = {
        "given", "taken", "made", "done", "seen", "known",
        "written", "spoken", "broken", "chosen", "driven",
        "eaten", "fallen", "found", "got", "gotten", "held",
        "kept", "left", "lost", "met", "paid", "put",
        "read", "said", "sent", "set", "shown", "sold",
        "spent", "told", "thought", "understood", "won",
        "brought", "bought", "caught", "taught", "built",
        "felt", "heard", "meant", "led", "dealt", "slept"
    };
    return irregular.count(word_lower) > 0;
}

// ------------------------------------------------------------
// 提取
// ------------------------------------------------------------
void SyntacticFeatures::extract(const Document& doc, FeatureVector& fv) {
    // 收集所有词（小写，已去首尾标点）
    std::vector<std::string> words;
    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            for (const auto& t : s.tokens) {
                if (t.word.empty()) continue;

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

    const std::size_t N = words.size();
    if (N == 0) return;

    // ---- 被动语态估计 ----
    // 规则：be 动词 + 2 词以内出现过去分词
    // 简化为：扫描每个词，如果遇到 be 动词，向后看 3 个词找 past participle
    std::size_t passive_cnt = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (beVerbs().count(words[i]) > 0) {
            for (std::size_t j = i + 1; j < N && j <= i + 3; ++j) {
                if (looksLikePastParticiple(words[j])) {
                    ++passive_cnt;
                    break;
                }
            }
        }
    }
    fv.passive_ratio = static_cast<double>(passive_cnt) /
                       static_cast<double>(N);

    // ---- 从句标记词 ----
    std::size_t clause_cnt = 0;
    for (const auto& w : words) {
        if (clauseMarkers().count(w) > 0) ++clause_cnt;
    }
    fv.clause_marker_ratio = static_cast<double>(clause_cnt) /
                             static_cast<double>(N);

    // ---- 情态动词 ----
    std::size_t modal_cnt = 0;
    for (const auto& w : words) {
        if (modalVerbs().count(w) > 0) ++modal_cnt;
    }
    fv.modal_ratio = static_cast<double>(modal_cnt) /
                     static_cast<double>(N);

    // ---- 名词化 ----
    std::size_t nominalization_cnt = 0;
    for (const auto& w : words) {
        if (w.size() < 6) continue;  // 太短不考虑
        for (const auto& suf : nominalizationSuffixes()) {
            if (utils::endsWith(w, suf)) {
                ++nominalization_cnt;
                break;
            }
        }
    }
    fv.nominalization_ratio = static_cast<double>(nominalization_cnt) /
                              static_cast<double>(N);

    // ---- -ing 形式 ----
    std::size_t gerund_cnt = 0;
    for (const auto& w : words) {
        if (w.size() >= 5 && utils::endsWith(w, "ing")) {
            ++gerund_cnt;
        }
    }
    fv.gerund_ratio = static_cast<double>(gerund_cnt) /
                      static_cast<double>(N);
}

// ------------------------------------------------------------
// 单句被动判断（简化）
// ------------------------------------------------------------
bool SyntacticFeatures::isPassiveSentence(const Sentence& s) {
    if (s.tokens.size() < 2) return false;
    for (std::size_t i = 0; i < s.tokens.size(); ++i) {
        std::string w = utils::toLower(s.tokens[i].word);
        if (beVerbs().count(w) > 0) {
            for (std::size_t j = i + 1;
                 j < s.tokens.size() && j <= i + 3; ++j) {
                std::string w2 = utils::toLower(s.tokens[j].word);
                if (looksLikePastParticiple(w2)) {
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace features
} // namespace calwf