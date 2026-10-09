// ============================================================
// CALWF - ReportGenerator 报告生成器
// ============================================================
//
// 文件名：include/calwf/report/ReportGenerator.h
// 作用：把一批 AnalysisResult 渲染成不同格式的报告。
//
// 支持格式：
//   - Markdown（适合 GitHub / VS Code 阅读）
//   - HTML（自带 CSS，浏览器打开，带可视化）
//   - CSV（适合 Excel / SPSS / R）
//
// 每个格式都有两个方法：
//   render        —— 返回字符串
//   save          —— 直接写文件
// ============================================================

#ifndef CALWF_REPORT_REPORTGENERATOR_H
#define CALWF_REPORT_REPORTGENERATOR_H

#include <string>
#include <vector>

#include "calwf/analysis/AnalysisResult.h"

namespace calwf {
namespace report {

class ReportGenerator {
public:
    // ============ Markdown ============
    static std::string renderMarkdown(
        const std::vector<analysis::AnalysisResult>& results,
        const std::string& title);
    static bool saveMarkdown(
        const std::vector<analysis::AnalysisResult>& results,
        const std::string& title,
        const std::string& path);

    // ============ HTML ============
    static std::string renderHtml(
        const std::vector<analysis::AnalysisResult>& results,
        const std::string& title);
    static bool saveHtml(
        const std::vector<analysis::AnalysisResult>& results,
        const std::string& title,
        const std::string& path);

    // ============ CSV ============
    static std::string renderCsv(
        const std::vector<analysis::AnalysisResult>& results);
    static bool saveCsv(
        const std::vector<analysis::AnalysisResult>& results,
        const std::string& path);

private:
    // 生成 ASCII 进度条（如 "████████░░ 80%"）
    static std::string bar(double ratio, int width = 20);

    // HTML 转义
    static std::string htmlEscape(const std::string& s);

    // CSV 转义
    static std::string csvEscape(const std::string& s);

    // 根据判定返回 HTML 颜色类名
    static std::string verdictClass(const std::string& verdict);

    // 根据风险等级返回 HTML 颜色类名
    static std::string riskClass(const std::string& risk);
};

} // namespace report
} // namespace calwf

#endif // CALWF_REPORT_REPORTGENERATOR_H