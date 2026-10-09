// ============================================================
// CALWF - CorpusAnchor 实现
// ============================================================

#include "calwf/features/CorpusAnchor.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/Logger.h"
#include "calwf/utils/StringUtils.h"

#include <cmath>
#include <cstring>
#include <fstream>

namespace calwf {
namespace features {

// ============================================================
// 工具函数
// ============================================================

// 清理一个词：去首尾标点，转小写
static std::string cleanWord(const std::string& raw) {
    std::size_t start = 0;
    std::size_t end   = raw.size();
    while (start < end && !utils::isAlpha(raw[start])) ++start;
    while (end > start && !utils::isAlpha(raw[end - 1])) --end;
    if (start >= end) return "";
    return utils::toLower(raw.substr(start, end - start));
}

std::vector<std::string> CorpusAnchor::docToWords(const Document& doc) {
    std::vector<std::string> words;
    words.reserve(static_cast<std::size_t>(doc.word_count));

    for (const auto& p : doc.paragraphs) {
        for (const auto& s : p.sentences) {
            for (const auto& t : s.tokens) {
                std::string w = cleanWord(t.word);
                if (!w.empty()) words.push_back(std::move(w));
            }
        }
    }
    return words;
}

std::string CorpusAnchor::makeBigramKey(const std::string& a,
                                        const std::string& b) {
    std::string key;
    key.reserve(a.size() + b.size() + 1);
    key += a;
    key += ' ';
    key += b;
    return key;
}

std::string CorpusAnchor::makeTrigramKey(const std::string& a,
                                         const std::string& b,
                                         const std::string& c) {
    std::string key;
    key.reserve(a.size() + b.size() + c.size() + 2);
    key += a;
    key += ' ';
    key += b;
    key += ' ';
    key += c;
    return key;
}

// ============================================================
// 构建
// ============================================================
void CorpusAnchor::build(const Corpus& corpus) {
    unigram_.clear();
    bigram_.clear();
    trigram_.clear();
    total_tokens_ = 0;

    std::uint64_t doc_count = 0;
    for (const auto& doc : corpus.docs) {
        auto words = docToWords(doc);
        const std::size_t N = words.size();
        if (N == 0) continue;
        ++doc_count;

        // unigram
        for (const auto& w : words) {
            ++unigram_[w];
            ++total_tokens_;
        }

        // bigram
        for (std::size_t i = 0; i + 2 <= N; ++i) {
            ++bigram_[makeBigramKey(words[i], words[i + 1])];
        }

        // trigram
        for (std::size_t i = 0; i + 3 <= N; ++i) {
            ++trigram_[makeTrigramKey(words[i], words[i + 1], words[i + 2])];
        }

        if (doc_count % 500 == 0) {
            CALWF_LOG_INFO("CorpusAnchor 构建中：已处理 %llu 篇",
                           static_cast<unsigned long long>(doc_count));
        }
    }

    CALWF_LOG_INFO("CorpusAnchor 构建完成："
                   "%zu unigram, %zu bigram, %zu trigram, %llu tokens",
                   unigram_.size(),
                   bigram_.size(),
                   trigram_.size(),
                   static_cast<unsigned long long>(total_tokens_));
}

// ============================================================
// 序列化
// ============================================================
// 二进制格式：
//   magic   8 字节  "CALWFNG1"
//   uint64  total_tokens
//   uint64  unigram_count
//   uint64  bigram_count
//   uint64  trigram_count
//   --- 各表：每项
//     uint32 len
//     bytes  key
//     uint32 freq
// ============================================================

static void writeStr(std::ofstream& out, const std::string& s) {
    std::uint32_t len = static_cast<std::uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(s.data(), len);
}

static bool readStr(std::ifstream& in, std::string& s) {
    std::uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!in) return false;
    s.resize(len);
    in.read(s.data(), len);
    return static_cast<bool>(in);
}

template <typename Map>
static void writeMap(std::ofstream& out, const Map& m) {
    std::uint64_t n = m.size();
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (const auto& kv : m) {
        writeStr(out, kv.first);
        std::uint32_t freq = kv.second;
        out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    }
}

template <typename Map>
static bool readMap(std::ifstream& in, Map& m) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    if (!in) return false;
    m.reserve(static_cast<std::size_t>(n));
    for (std::uint64_t i = 0; i < n; ++i) {
        std::string key;
        if (!readStr(in, key)) return false;
        std::uint32_t freq = 0;
        in.read(reinterpret_cast<char*>(&freq), sizeof(freq));
        if (!in) return false;
        m.emplace(std::move(key), freq);
    }
    return true;
}

bool CorpusAnchor::save(const std::string& path) const {
    // 确保目录存在
    std::string dir = utils::dirName(path);
    if (!dir.empty()) utils::createDirs(dir);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        CALWF_LOG_ERROR("CorpusAnchor 保存失败：%s", path.c_str());
        return false;
    }

    const char magic[8] = {'C','A','L','W','F','N','G','1'};
    out.write(magic, 8);

    out.write(reinterpret_cast<const char*>(&total_tokens_), sizeof(total_tokens_));

    writeMap(out, unigram_);
    writeMap(out, bigram_);
    writeMap(out, trigram_);

    CALWF_LOG_INFO("CorpusAnchor 已保存：%s", path.c_str());
    return out.good();
}

bool CorpusAnchor::load(const std::string& path) {
    if (!utils::fileExists(path)) return false;

    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    char magic[8];
    in.read(magic, 8);
    if (std::memcmp(magic, "CALWFNG1", 8) != 0) {
        CALWF_LOG_ERROR("CorpusAnchor 加载失败：magic 不匹配");
        return false;
    }

    in.read(reinterpret_cast<char*>(&total_tokens_), sizeof(total_tokens_));

    unigram_.clear();
    bigram_.clear();
    trigram_.clear();

    if (!readMap(in, unigram_)) return false;
    if (!readMap(in, bigram_))  return false;
    if (!readMap(in, trigram_)) return false;

    CALWF_LOG_INFO("CorpusAnchor 已加载：%s （%zu unigram, %zu bigram, %zu trigram）",
                   path.c_str(),
                   unigram_.size(),
                   bigram_.size(),
                   trigram_.size());
    return true;
}

// ============================================================
// 特征计算
// ============================================================
double CorpusAnchor::ngramCoverage(const Document& doc, int n) const {
    auto words = docToWords(doc);
    const std::size_t N = words.size();

    if (n == 1) {
        if (N == 0) return 0.0;
        std::size_t hit = 0;
        for (const auto& w : words) {
            if (unigram_.find(w) != unigram_.end()) ++hit;
        }
        return static_cast<double>(hit) / static_cast<double>(N);
    }

    if (n == 2) {
        if (N < 2) return 0.0;
        std::size_t total = 0;
        std::size_t hit   = 0;
        for (std::size_t i = 0; i + 2 <= N; ++i) {
            ++total;
            if (bigram_.find(makeBigramKey(words[i], words[i + 1]))
                != bigram_.end()) {
                ++hit;
            }
        }
        return (total == 0) ? 0.0
                            : static_cast<double>(hit) /
                              static_cast<double>(total);
    }

    if (n == 3) {
        if (N < 3) return 0.0;
        std::size_t total = 0;
        std::size_t hit   = 0;
        for (std::size_t i = 0; i + 3 <= N; ++i) {
            ++total;
            if (trigram_.find(makeTrigramKey(words[i], words[i + 1], words[i + 2]))
                != trigram_.end()) {
                ++hit;
            }
        }
        return (total == 0) ? 0.0
                            : static_cast<double>(hit) /
                              static_cast<double>(total);
    }

    return 0.0;
}

double CorpusAnchor::oovRate(const Document& doc) const {
    auto words = docToWords(doc);
    if (words.empty()) return 0.0;

    std::size_t oov = 0;
    for (const auto& w : words) {
        if (unigram_.find(w) == unigram_.end()) ++oov;
    }
    return static_cast<double>(oov) / static_cast<double>(words.size());
}

double CorpusAnchor::cosineSimilarity(const Document& doc) const {
    auto words = docToWords(doc);
    if (words.empty()) return 0.0;

    // 文档词频
    std::unordered_map<std::string, std::uint32_t> doc_freq;
    doc_freq.reserve(words.size());
    for (const auto& w : words) {
        ++doc_freq[w];
    }

    // 计算分子（点积）：只对共同词
    double dot = 0.0;
    for (const auto& kv : doc_freq) {
        auto it = unigram_.find(kv.first);
        if (it != unigram_.end()) {
            dot += static_cast<double>(kv.second) *
                   static_cast<double>(it->second);
        }
    }

    // 分母：|doc| * |corpus|
    double doc_norm_sq = 0.0;
    for (const auto& kv : doc_freq) {
        doc_norm_sq += static_cast<double>(kv.second) *
                       static_cast<double>(kv.second);
    }

    // 语料库范数：只对文档出现的词求和（因为其他词在点积里贡献 0）
    double corpus_norm_sq = 0.0;
    for (const auto& kv : doc_freq) {
        auto it = unigram_.find(kv.first);
        if (it != unigram_.end()) {
            corpus_norm_sq += static_cast<double>(it->second) *
                              static_cast<double>(it->second);
        }
    }

    if (doc_norm_sq <= 0.0 || corpus_norm_sq <= 0.0) return 0.0;
    return dot / (std::sqrt(doc_norm_sq) * std::sqrt(corpus_norm_sq));
}

} // namespace features
} // namespace calwf