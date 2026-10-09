// ============================================================
// CALWF - Paragraph 定义
// ============================================================
//
// 文件名：include/calwf/core/Paragraph.h
// 作用：定义段落结构，包含原文和 Sentence 序列。
//
// 对应数据结构课程知识：类 / 封装 / 线性表（vector）
// ============================================================

#ifndef CALWF_CORE_PARAGRAPH_H
#define CALWF_CORE_PARAGRAPH_H

#include <string>
#include <vector>

#include "calwf/core/Sentence.h"

namespace calwf {

// ------------------------------------------------------------
// Paragraph：一个段落
// ------------------------------------------------------------
struct Paragraph {
    // ---- 位置信息 ----
    int id = -1;   // 段落在文档中的编号（从 1 开始）

    // ---- 内容 ----
    std::string text;                    // 段落原文
    std::vector<Sentence> sentences;     // 句子序列

    // ---- 便捷方法 ----

    // 句数
    int sentenceCount() const {
        return static_cast<int>(sentences.size());
    }

    // 词数（累加每个句子）
    int wordCount() const {
        int total = 0;
        for (const auto& s : sentences) {
            total += s.wordCount();
        }
        return total;
    }

    bool empty() const {
        return text.empty() && sentences.empty();
    }
};

} // namespace calwf

#endif // CALWF_CORE_PARAGRAPH_H