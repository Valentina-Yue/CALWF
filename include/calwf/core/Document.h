// ============================================================
// CALWF - Document 定义
// ============================================================
//
// 文件名：include/calwf/core/Document.h
// 作用：定义一个完整的文档，统一 SWECCL / BAWE / LLM 三种来源。
//
// 说明：
//   - 三种数据源在字段上有差异：
//       SWECCL  : 有 genre、level、院校代码，无 L1、无 POS
//       BAWE    : 有 genre_family、discipline、L1、POS
//       LLM     : 有 model_name、prompt_id、deep_thinking
//   - 我们用统一的 Document 承载所有字段，
//     不适用的字段留空或使用默认值。
//
// 对应数据结构课程知识：类 / 封装 / 线性表（vector）
// ============================================================

#ifndef CALWF_CORE_DOCUMENT_H
#define CALWF_CORE_DOCUMENT_H

#include <string>
#include <vector>

#include "calwf/core/Paragraph.h"

namespace calwf {

// ------------------------------------------------------------
// 数据来源枚举
// ------------------------------------------------------------
enum class SourceType {
    Unknown,    // 未知
    SWECCL,     // 中国学生英语口笔语语料库（笔语部分）
    BAWE,       // 英国学术写作语料库
    LLM         // 大模型输出
};

// 把枚举转成字符串（用于报告输出）
inline const char* sourceTypeName(SourceType t) {
    switch (t) {
        case SourceType::SWECCL: return "SWECCL";
        case SourceType::BAWE:   return "BAWE";
        case SourceType::LLM:    return "LLM";
        default:                 return "Unknown";
    }
}

// ------------------------------------------------------------
// Document：一个完整的文档
// ------------------------------------------------------------
class Document {
public:
    // ============ 身份 ============
    std::string doc_id;                 // 唯一 ID，如 "SWECCL_ARG_A101FYSY"
    SourceType  source = SourceType::Unknown;   // 数据来源

    // ============ 元数据（通用） ============
    std::string genre;                  // 文体：ARG / EXP / NAR / essay / ...
    std::string genre_family;           // 仅 BAWE：13 类 genre family
    int         level = -1;             // 年级/水平：1-4，-1 表示未设置

    // ============ 元数据（BAWE 特有） ============
    std::string discipline;             // 具体学科，如 "Sociology"
    std::string disciplinary_group;     // AH / LS / PS / SS
    std::string l1;                     // 母语，如 "English"、"Chinese Cantonese"
    std::string gender;                 // "m" / "f"
    int         year_of_birth = -1;
    std::string education;              // "UKa" / "OSa" / "UK1" 等

    // ============ 元数据（SWECCL 特有） ============
    std::string school_code;            // 院校代码，如 "FYSY"
    bool        timed = false;          // 是否限时作文

    // ============ 元数据（LLM 特有） ============
    std::string model_name;             // "DeepSeek" / "Doubao" / ...
    std::string model_version;          // 如 "v3"、"K2"
    std::string prompt_id;              // 如 "p001"
    std::string topic;                  // 题目
    std::string task_type;              // "Argumentation" / "Exposition" / "Narration"
    std::string target_level;           // "CET4" / "CET6" / "TEM4" / "TEM8"
    int         run_number = -1;        // 第几次生成
    bool        deep_thinking = false;  // 是否开启深度思考
    bool        web_access = false;     // 是否联网
    std::string date_generated;         // 生成日期

    // ============ 统计 ============
    int word_count = 0;
    int sentence_count = 0;
    int paragraph_count = 0;

    // ============ 内容 ============
    std::string raw_text;                       // 原始全文
    std::vector<Paragraph> paragraphs;          // 段落序列

    // ============ 便捷方法 ============

    // 全部句子（展平）
    std::vector<std::string> allSentences() const {
        std::vector<std::string> result;
        for (const auto& p : paragraphs) {
            for (const auto& s : p.sentences) {
                result.push_back(s.text);
            }
        }
        return result;
    }

    // 全部词（展平，仅 word 字符串）
    std::vector<std::string> allWords() const {
        std::vector<std::string> result;
        for (const auto& p : paragraphs) {
            for (const auto& s : p.sentences) {
                for (const auto& t : s.tokens) {
                    result.push_back(t.word);
                }
            }
        }
        return result;
    }

    // 统计（如果不一致，用这个方法重新计算）
    void recomputeStats() {
        word_count = 0;
        sentence_count = 0;
        paragraph_count = static_cast<int>(paragraphs.size());
        for (const auto& p : paragraphs) {
            sentence_count += p.sentenceCount();
            word_count += p.wordCount();
        }
    }

    bool empty() const {
        return paragraphs.empty();
    }
};

} // namespace calwf

#endif // CALWF_CORE_DOCUMENT_H