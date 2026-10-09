// ============================================================
// CALWF - SWECCL 标签解析
// ============================================================
//
// 文件名：include/calwf/io/SwecclMeta.h
// 作用：解析 SWECCL 文本首行的尖括号标签。
//
// 标签格式示例：
//   <WCOMP><ARG><GRADE1><YEAR03><TIMED><SCORE?><FYSY><LENGTH300W>
//
// 标签含义：
//   <WCOMP>      Written Composition
//   <ARG/EXP/NAR> 议论文/说明文/记叙文
//   <GRADE1-4>   年级
//   <YEAR03>     2003 级
//   <TIMED/UNTIMED>  限时/非限时
//   <SCORE?>     分数未知
//   <FYSY>       院校代码
//   <LENGTH300W> 目标词数
//
// 对应数据结构课程知识：串 / 哈希表
// ============================================================

#ifndef CALWF_IO_SWECCLMETA_H
#define CALWF_IO_SWECCLMETA_H

#include <string>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// SWECCL 标签结构
// ------------------------------------------------------------
struct SwecclTags {
    bool        is_wcomp    = false;   // <WCOMP>
    std::string genre;                 // ARG / EXP / NAR
    int         grade       = -1;      // 1-4
    std::string year_code;             // 如 "03"（从 <YEAR03> 提取）
    bool        is_timed    = false;   // 限时
    bool        is_untimed  = false;   // 非限时
    std::string school_code;           // FYSY / HZKD / JND / ND / NGD / NGG / SD / XSD / NGSD 等
    int         target_length = -1;    // <LENGTH300W> → 300
    bool        has_score   = false;   // <SCORE?> 存在
};

// ------------------------------------------------------------
// 解析器
// ------------------------------------------------------------
class SwecclTagParser {
public:
    // 解析一行标签，返回是否成功
    static bool parse(const std::string& line, SwecclTags& tags);

    // 清洗正文：
    //   - 删除 <gr-xxx>、<sp-xxx>、<mis-xxx>
    //   - 删除 <a>
    //   - 去掉多余空格
    static std::string cleanText(const std::string& text);
};

} // namespace io
} // namespace calwf

#endif // CALWF_IO_SWECCLMETA_H