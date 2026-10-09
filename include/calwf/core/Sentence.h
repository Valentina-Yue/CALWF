// ============================================================
// CALWF - Sentence 定义
// ============================================================
//
// 文件名：include/calwf/core/Sentence.h
// 作用：定义句子结构，包含原文和 Token 序列。
//
// 对应数据结构课程知识：类 / 封装 / 线性表（vector）
// ============================================================

#ifndef CALWF_CORE_SENTENCE_H
#define CALWF_CORE_SENTENCE_H

#include <string>
#include <vector>

#include "calwf/core/Token.h"

namespace calwf {

// ------------------------------------------------------------
// Sentence：一个句子
// ------------------------------------------------------------
struct Sentence {
    // ---- 位置信息 ----
    int id = -1;   // 句子在文档中的编号（从 1 开始）

    // ---- 内容 ----
    std::string text;              // 句子原文
    std::vector<Token> tokens;     // 逐词序列（可能为空）

    // ---- 便捷方法 ----

    // 词数（如果没有 token，则按空格粗略统计）
    int wordCount() const {
        if (!tokens.empty()) {
            return static_cast<int>(tokens.size());
        }
        // 粗略：按空格分隔
        int count = 0;
        bool in_word = false;
        for (char c : text) {
            if (c == ' ' || c == '\t' || c == '\n') {
                in_word = false;
            } else if (!in_word) {
                in_word = true;
                ++count;
            }
        }
        return count;
    }

    // 字符数
    int charCount() const {
        return static_cast<int>(text.size());
    }

    bool empty() const {
        return text.empty() && tokens.empty();
    }
};

} // namespace calwf

#endif // CALWF_CORE_SENTENCE_H