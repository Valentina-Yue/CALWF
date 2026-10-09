// ============================================================
// CALWF - Corpus 定义
// ============================================================
//
// 文件名：include/calwf/core/Corpus.h
// 作用：语料库容器，管理一组 Document，并提供快速查找和筛选。
//
// 说明：
//   - 内部用 vector<Document> 保存所有文档（线性表）。
//   - 用 unordered_map 建立 doc_id → 索引的哈希表，
//     使得按 ID 查找是 O(1)。
//
// 对应数据结构课程知识：
//   - 类 / 封装
//   - 线性表（vector）
//   - 哈希表（unordered_map）
//   - 查找（按 ID 查找）
// ============================================================

#ifndef CALWF_CORE_CORPUS_H
#define CALWF_CORE_CORPUS_H

#include <string>
#include <vector>
#include <unordered_map>

#include "calwf/core/Document.h"

namespace calwf {

// ------------------------------------------------------------
// Corpus：语料库
// ------------------------------------------------------------
class Corpus {
public:
    // ---- 数据 ----
    std::vector<Document> docs;   // 所有文档（线性表）
    std::unordered_map<std::string, int> id_index;   // doc_id → docs 下标

    // ---- 方法 ----

    // 添加一个文档（移动语义，避免拷贝）
    void add(Document doc) {
        // docs.size() 返回 std::size_t，显式转成 int 以匹配 id_index 的值类型
        const int idx = static_cast<int>(docs.size());
        id_index[doc.doc_id] = idx;
        docs.push_back(std::move(doc));
    }

    // 按 ID 查找（返回指针，未找到返回 nullptr）
    const Document* find(const std::string& doc_id) const {
        auto it = id_index.find(doc_id);
        if (it == id_index.end()) {
            return nullptr;
        }
        // 显式转回 std::size_t，避免 sign-conversion 警告
        return &docs[static_cast<std::size_t>(it->second)];
    }

    // 文档数量
    std::size_t size() const {
        return docs.size();
    }

    bool empty() const {
        return docs.empty();
    }

    // 按条件筛选，返回指针列表
    //
    // 参数：
    //   source_filter     —— 数据来源，SourceType::Unknown 表示不筛选
    //   genre_filter      —— 文体，空字符串表示不筛选
    //   level_filter      —— 年级，-1 表示不筛选
    //   disc_group_filter —— 学科组，空字符串表示不筛选
    std::vector<const Document*> filter(
        SourceType source_filter = SourceType::Unknown,
        const std::string& genre_filter = "",
        int level_filter = -1,
        const std::string& disc_group_filter = ""
    ) const {
        std::vector<const Document*> result;
        result.reserve(docs.size());

        for (const auto& d : docs) {
            if (source_filter != SourceType::Unknown && d.source != source_filter) {
                continue;
            }
            if (!genre_filter.empty() && d.genre != genre_filter) {
                continue;
            }
            if (level_filter >= 0 && d.level != level_filter) {
                continue;
            }
            if (!disc_group_filter.empty() &&
                d.disciplinary_group != disc_group_filter) {
                continue;
            }
            result.push_back(&d);
        }
        return result;
    }

    // 统计信息（用于报告）
    struct Stats {
        int total = 0;
        int sweccl = 0;
        int bawe   = 0;
        int llm    = 0;
        int total_words = 0;
        int total_sentences = 0;
    };

    Stats stats() const {
        Stats s;
        s.total = static_cast<int>(docs.size());
        for (const auto& d : docs) {
            switch (d.source) {
                case SourceType::SWECCL: ++s.sweccl; break;
                case SourceType::BAWE:   ++s.bawe;   break;
                case SourceType::LLM:    ++s.llm;    break;
                default: break;
            }
            s.total_words     += d.word_count;
            s.total_sentences += d.sentence_count;
        }
        return s;
    }
};

} // namespace calwf

#endif // CALWF_CORE_CORPUS_H