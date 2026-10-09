// ============================================================
// CALWF - Token 定义
// ============================================================
//
// 文件名：include/calwf/core/Token.h
// 作用：定义文本分析的最小单位——Token（词/符号）。
//
// 说明：
//   - BAWE 语料库的 CSV 已经提供了逐词信息（词形、词元、词性、
//     命名实体、依存关系），所以我们的 Token 结构与 BAWE 对齐。
//   - SWECCL 没有 POS 标注，Token 的 pos/lemma/ner/dep 字段会留空。
//   - LLM 输出也没有标注，由我们的 Tokenizer 填充 word 字段，
//     pos/lemma 等字段留空或由简单规则填充。
//
// 对应数据结构课程知识：类 / 封装 / 线性表
// ============================================================

#ifndef CALWF_CORE_TOKEN_H
#define CALWF_CORE_TOKEN_H

#include <string>

namespace calwf {

// ------------------------------------------------------------
// Token：文本的最小分析单位
// ------------------------------------------------------------
struct Token {
    // ---- 位置信息 ----
    int sentence_id = -1;   // 所属句子的编号（从 1 开始，0 或 -1 表示未设置）
    int word_id     = -1;   // 在本句中的编号（从 1 开始）

    // ---- 词形信息 ----
    std::string word;       // 原词，如 "Racism"
    std::string lemma;      // 词元，如 "racism"
    std::string pos;        // 词性，如 "NN"、"VBZ"
    std::string ner;        // 命名实体，如 "MISC"、"O"

    // ---- 依存信息（仅 BAWE 有） ----
    int         dep_on = -1;   // 依存父节点的 word_id
    std::string dep;           // 依存关系，如 "nsubj"、"ROOT"

    // ---- 便捷方法 ----
    bool empty() const {
        return word.empty();
    }

    // 是否为大写开头（用于统计句子边界等）
    bool startsWithUpper() const {
        return !word.empty() &&
               word[0] >= 'A' && word[0] <= 'Z';
    }
};

} // namespace calwf

#endif // CALWF_CORE_TOKEN_H