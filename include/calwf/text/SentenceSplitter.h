// ============================================================
// CALWF - SentenceSplitter 分句器
// ============================================================
//
// 文件名：include/calwf/text/SentenceSplitter.h
// 作用：把一段文本按标点切分成句子。
//
// 规则（简化版，够用即可）：
//   - 强标点（. ! ?）后如果跟空格 + 大写字母或数字，则切分。
//   - 强标点后如果跟引号 + 空格 + 大写，则切分。
//   - 常见缩写（Mr. / Dr. / etc. / e.g. / i.e. 等）后不切分。
//   - 不处理引号内的句号（简化处理）。
//
// 对应数据结构课程知识：串 / 状态机
// ============================================================

#ifndef CALWF_TEXT_SENTENCESPLITTER_H
#define CALWF_TEXT_SENTENCESPLITTER_H

#include <string>
#include <vector>

namespace calwf {
namespace text {

class SentenceSplitter {
public:
    // 把一段文本切分成句子
    static std::vector<std::string> split(const std::string& text);

    // 把整个文档按空行切段落，每段再分句
    // 返回：段落列表，每段是句子列表
    static std::vector<std::vector<std::string>>
    splitParagraphs(const std::string& text);

private:
    // 判断某个位置是否构成切分点
    static bool isBoundary(const std::string& text, std::size_t pos);

    // 判断从 pos 开始的单词是否是常见缩写
    static bool isAbbreviation(const std::string& text, std::size_t pos);
};

} // namespace text
} // namespace calwf

#endif // CALWF_TEXT_SENTENCESPLITTER_H