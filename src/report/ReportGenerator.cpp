// ============================================================
// CALWF - ReportGenerator 实现
// ============================================================

#include "calwf/report/ReportGenerator.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <map>
#include <sstream>

namespace calwf {
namespace report {

// ------------------------------------------------------------
// 工具函数
// ------------------------------------------------------------
std::string ReportGenerator::bar(double ratio, int width) {
    ratio = std::max(0.0, std::min(1.0, ratio));
    int filled = static_cast<int>(ratio * width + 0.5);
    std::string s;
    for (int i = 0; i < width; ++i) {
        s += (i < filled) ? "#" : ".";
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), " %5.1f%%", ratio * 100.0);
    return s + buf;
}

std::string ReportGenerator::htmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string ReportGenerator::csvEscape(const std::string& s) {
    bool need = false;
    for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            need = true;
            break;
        }
    }
    if (!need) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

std::string ReportGenerator::verdictClass(const std::string& verdict) {
    if (verdict == "AI")        return "verdict-ai";
    if (verdict == "Human")     return "verdict-human";
    return "verdict-uncertain";
}

std::string ReportGenerator::riskClass(const std::string& risk) {
    if (risk == "High")   return "risk-high";
    if (risk == "Medium") return "risk-medium";
    return "risk-low";
}

// ============================================================
// 统计聚合
// ============================================================
namespace {

struct ModelStat {
    std::string model;
    int         count = 0;
    int         ai_count = 0;
    int         human_count = 0;
    int         uncertain_count = 0;
    double      sum_suspicion = 0.0;
    double      sum_human = 0.0;
    double      sum_sweccl = 0.0;
    double      sum_bawe = 0.0;
};

std::vector<ModelStat> aggregate(
    const std::vector<analysis::AnalysisResult>& results,
    const std::vector<std::string>& model_order) {

    std::map<std::string, ModelStat> m;
    for (const auto& name : model_order) {
        m[name].model = name;
    }

    for (const auto& r : results) {
        // 从 doc_id 提取模型名：LLM_<Model>_pXXX_rN
        std::string model = "Unknown";
        // 找第一个 '_' 之后、第二个 '_' 之前
        auto p1 = r.doc_id.find('_');
        if (p1 != std::string::npos) {
            auto p2 = r.doc_id.find('_', p1 + 1);
            if (p2 != std::string::npos) {
                model = r.doc_id.substr(p1 + 1, p2 - p1 - 1);
            }
        }

        auto& s = m[model];
        s.model = model;
        ++s.count;
        s.sum_suspicion += r.ai_suspicion;
        s.sum_human     += r.human_likeness;
        s.sum_sweccl    += r.sweccl_anchoring;
        s.sum_bawe      += r.bawe_anchoring;
        if (r.verdict == "AI")           ++s.ai_count;
        else if (r.verdict == "Human")   ++s.human_count;
        else                              ++s.uncertain_count;
    }

    std::vector<ModelStat> result;
    for (const auto& name : model_order) {
        if (m.count(name)) result.push_back(m[name]);
    }
    // 加上未知的
    for (const auto& kv : m) {
        if (std::find(model_order.begin(), model_order.end(), kv.first)
            == model_order.end()) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::string timestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

} // anonymous namespace

// ============================================================
// Markdown
// ============================================================
std::string ReportGenerator::renderMarkdown(
    const std::vector<analysis::AnalysisResult>& results,
    const std::string& title) {

    std::ostringstream md;
    md << "# " << title << "\n\n";
    md << "生成时间：" << timestamp() << "\n\n";
    md << "文档总数：" << results.size() << "\n\n";
    md << "---\n\n";

    // ---------- 1. 概览表 ----------
    std::vector<std::string> model_order = {
        "DeepSeek", "Doubao", "ChatGLM", "Kimi", "Qwen"
    };
    auto stats = aggregate(results, model_order);

    md << "## 模型概览\n\n";
    md << "| 模型 | 篇数 | 判AI | 判Human | 不确定 | 平均AI嫌疑 | 平均SWECCL锚定 | 平均BAWE锚定 |\n";
    md << "|---|---|---|---|---|---|---|---|\n";

    char buf[512];
    for (const auto& s : stats) {
        if (s.count == 0) continue;
        double n = static_cast<double>(s.count);
        std::snprintf(buf, sizeof(buf),
            "| %s | %d | %d | %d | %d | %.2f%% | %.2f%% | %.2f%% |\n",
            s.model.c_str(), s.count,
            s.ai_count, s.human_count, s.uncertain_count,
            100.0 * s.sum_suspicion / n,
            100.0 * s.sum_sweccl / n,
            100.0 * s.sum_bawe / n);
        md << buf;
    }
    md << "\n";

    // ---------- 2. 单篇详情 ----------
    md << "## 逐篇分析\n\n";
    int idx = 0;
    for (const auto& r : results) {
        ++idx;
        md << "### " << idx << ". " << r.doc_id << "\n\n";

        md << "| 指标 | 值 |\n";
        md << "|---|---|\n";
        std::snprintf(buf, sizeof(buf), "| AI 嫌疑度 | **%.2f%%** |\n",
                      100.0 * r.ai_suspicion);
        md << buf;
        std::snprintf(buf, sizeof(buf), "| 人味指数 | %.2f%% |\n",
                      100.0 * r.human_likeness);
        md << buf;
        std::snprintf(buf, sizeof(buf), "| SWECCL 锚定 | %.2f%% |\n",
                      100.0 * r.sweccl_anchoring);
        md << buf;
        std::snprintf(buf, sizeof(buf), "| BAWE 锚定 | %.2f%% |\n",
                      100.0 * r.bawe_anchoring);
        md << buf;
        md << "| 判定 | **" << r.verdict << "** |\n";
        md << "| 风险等级 | " << r.risk_level << " |\n\n";

        if (!r.model_ranking.empty()) {
            md << "**模型归因（top-3）**\n\n";
            for (const auto& m : r.model_ranking) {
                std::snprintf(buf, sizeof(buf),
                    "- %s：距离 %.2f，置信度 %.2f%%\n",
                    m.model_name.c_str(), m.distance,
                    100.0 * m.confidence);
                md << buf;
            }
            md << "\n";
        }

        if (!r.risk_reasons.empty()) {
            md << "**风险原因**\n\n";
            for (const auto& s : r.risk_reasons) {
                md << "- " << s << "\n";
            }
            md << "\n";
        }

        if (!r.risk_hits.empty()) {
            md << "**风险词命中**：";
            std::size_t n = std::min<std::size_t>(5, r.risk_hits.size());
            for (std::size_t i = 0; i < n; ++i) {
                if (i) md << "、";
                md << "`" << r.risk_hits[i] << "`";
            }
            md << "\n\n";
        }

        if (!r.top_sweccl_docs.empty()) {
            md << "**最相似 SWECCL 文档**：";
            for (std::size_t i = 0; i < r.top_sweccl_docs.size(); ++i) {
                if (i) md << "、";
                md << "`" << r.top_sweccl_docs[i] << "`";
            }
            md << "\n\n";
        }
        if (!r.top_bawe_docs.empty()) {
            md << "**最相似 BAWE 文档**：";
            for (std::size_t i = 0; i < r.top_bawe_docs.size(); ++i) {
                if (i) md << "、";
                md << "`" << r.top_bawe_docs[i] << "`";
            }
            md << "\n\n";
        }

        md << "---\n\n";
    }

    return md.str();
}

bool ReportGenerator::saveMarkdown(
    const std::vector<analysis::AnalysisResult>& results,
    const std::string& title,
    const std::string& path) {
    std::string content = renderMarkdown(results, title);
    std::string dir = utils::dirName(path);
    if (!dir.empty()) utils::createDirs(dir);
    return utils::writeFile(path, content);
}

// ============================================================
// HTML
// ============================================================
std::string ReportGenerator::renderHtml(
    const std::vector<analysis::AnalysisResult>& results,
    const std::string& title) {

    std::ostringstream h;
    h << "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n";
    h << "<meta charset=\"UTF-8\">\n";
    h << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    h << "<title>" << htmlEscape(title) << "</title>\n";
    h << "<style>\n";
    h << R"(
:root {
  --bg: #f6f8fa;
  --card: #ffffff;
  --text: #24292f;
  --muted: #656d76;
  --border: #d0d7de;
  --ai: #cf222e;
  --human: #1a7f37;
  --uncertain: #bf8700;
  --accent: #0969da;
}
* { box-sizing: border-box; }
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
               "PingFang SC", "Microsoft YaHei", sans-serif;
  background: var(--bg); color: var(--text);
  max-width: 1200px; margin: 0 auto; padding: 24px;
  line-height: 1.6;
}
h1 { border-bottom: 2px solid var(--border); padding-bottom: 12px; }
h2 { margin-top: 40px; border-bottom: 1px solid var(--border); padding-bottom: 8px; }
h3 { margin-top: 28px; color: var(--accent); }
.meta { color: var(--muted); font-size: 14px; margin-bottom: 24px; }
table {
  border-collapse: collapse; width: 100%; margin: 12px 0;
  background: var(--card); border: 1px solid var(--border);
  border-radius: 6px; overflow: hidden;
}
th, td { padding: 8px 12px; text-align: left; border-bottom: 1px solid var(--border); }
th { background: #f6f8fa; font-weight: 600; }
tr:last-child td { border-bottom: none; }
.card {
  background: var(--card); border: 1px solid var(--border);
  border-radius: 8px; padding: 20px; margin: 20px 0;
}
.bar {
  display: inline-block; height: 14px; border-radius: 3px;
  vertical-align: middle; margin-right: 8px;
}
.bar-bg { background: #eaeef2; width: 200px; }
.bar-fill { background: var(--accent); }
.verdict-ai { color: var(--ai); font-weight: 700; }
.verdict-human { color: var(--human); font-weight: 700; }
.verdict-uncertain { color: var(--uncertain); font-weight: 700; }
.risk-high { color: var(--ai); font-weight: 700; }
.risk-medium { color: var(--uncertain); font-weight: 700; }
.risk-low { color: var(--human); }
.tag {
  display: inline-block; padding: 2px 8px; border-radius: 12px;
  background: #eaeef2; font-size: 12px; margin: 2px;
  font-family: ui-monospace, Menlo, Consolas, monospace;
}
.footer {
  margin-top: 60px; padding-top: 20px;
  border-top: 1px solid var(--border);
  color: var(--muted); font-size: 13px; text-align: center;
}
)";
    h << "</style>\n</head>\n<body>\n";

    // ---- 标题 ----
    h << "<h1>" << htmlEscape(title) << "</h1>\n";
    h << "<div class=\"meta\">生成时间：" << timestamp()
      << " &middot; 文档总数：" << results.size() << "</div>\n";

    // ---- 概览 ----
    std::vector<std::string> model_order = {
        "DeepSeek", "Doubao", "ChatGLM", "Kimi", "Qwen"
    };
    auto stats = aggregate(results, model_order);

    h << "<h2>模型概览</h2>\n";
    h << "<table>\n<thead>\n<tr>";
    h << "<th>模型</th><th>篇数</th><th>判AI</th><th>判Human</th>"
      << "<th>不确定</th><th>平均AI嫌疑</th><th>SWECCL锚定</th><th>BAWE锚定</th>";
    h << "</tr>\n</thead>\n<tbody>\n";

    for (const auto& s : stats) {
        if (s.count == 0) continue;
        double n = static_cast<double>(s.count);
        double ai_pct = 100.0 * s.sum_suspicion / n;
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "<tr><td><strong>%s</strong></td><td>%d</td>"
            "<td>%d</td><td>%d</td><td>%d</td>"
            "<td><span class=\"bar bar-bg\"><span class=\"bar bar-fill\" "
            "style=\"width:%dpx\"></span></span>%.2f%%</td>"
            "<td>%.2f%%</td><td>%.2f%%</td></tr>\n",
            s.model.c_str(), s.count,
            s.ai_count, s.human_count, s.uncertain_count,
            static_cast<int>(ai_pct / 100.0 * 200.0),
            ai_pct,
            100.0 * s.sum_sweccl / n,
            100.0 * s.sum_bawe / n);
        h << buf;
    }
    h << "</tbody>\n</table>\n";

    // ---- 逐篇 ----
    h << "<h2>逐篇分析</h2>\n";
    int idx = 0;
    for (const auto& r : results) {
        ++idx;
        h << "<div class=\"card\">\n";
        h << "<h3>" << idx << ". " << htmlEscape(r.doc_id) << "</h3>\n";

        char buf[1024];
        std::snprintf(buf, sizeof(buf),
            "<table>\n<tr><th>AI 嫌疑度</th>"
            "<td><span class=\"bar bar-bg\"><span class=\"bar bar-fill\" "
            "style=\"width:%dpx\"></span></span><strong>%.2f%%</strong></td></tr>\n"
            "<tr><th>人味指数</th><td>%.2f%%</td></tr>\n"
            "<tr><th>SWECCL 锚定</th><td>%.2f%%</td></tr>\n"
            "<tr><th>BAWE 锚定</th><td>%.2f%%</td></tr>\n"
            "<tr><th>判定</th><td class=\"%s\">%s</td></tr>\n"
            "<tr><th>风险等级</th><td class=\"%s\">%s</td></tr>\n"
            "</table>\n",
            static_cast<int>(r.ai_suspicion * 200.0),
            100.0 * r.ai_suspicion,
            100.0 * r.human_likeness,
            100.0 * r.sweccl_anchoring,
            100.0 * r.bawe_anchoring,
            verdictClass(r.verdict).c_str(), r.verdict.c_str(),
            riskClass(r.risk_level).c_str(), r.risk_level.c_str());
        h << buf;

        // 模型归因
        if (!r.model_ranking.empty()) {
            h << "<h4>模型归因（top-3）</h4>\n<ul>\n";
            for (const auto& m : r.model_ranking) {
                std::snprintf(buf, sizeof(buf),
                    "<li><strong>%s</strong> 距离 %.2f "
                    "置信度 <strong>%.2f%%</strong></li>\n",
                    m.model_name.c_str(), m.distance,
                    100.0 * m.confidence);
                h << buf;
            }
            h << "</ul>\n";
        }

        // 风险
        if (!r.risk_reasons.empty()) {
            h << "<h4>风险原因</h4>\n<ul>\n";
            for (const auto& s : r.risk_reasons) {
                h << "<li>" << htmlEscape(s) << "</li>\n";
            }
            h << "</ul>\n";
        }

        if (!r.risk_hits.empty()) {
            h << "<h4>风险词命中</h4>\n<div>\n";
            std::size_t n = std::min<std::size_t>(5, r.risk_hits.size());
            for (std::size_t i = 0; i < n; ++i) {
                h << "<span class=\"tag\">"
                  << htmlEscape(r.risk_hits[i]) << "</span>\n";
            }
            h << "</div>\n";
        }

        if (!r.top_sweccl_docs.empty()) {
            h << "<h4>最相似 SWECCL 文档</h4>\n<div>\n";
            for (const auto& s : r.top_sweccl_docs) {
                h << "<span class=\"tag\">" << htmlEscape(s) << "</span>\n";
            }
            h << "</div>\n";
        }
        if (!r.top_bawe_docs.empty()) {
            h << "<h4>最相似 BAWE 文档</h4>\n<div>\n";
            for (const auto& s : r.top_bawe_docs) {
                h << "<span class=\"tag\">" << htmlEscape(s) << "</span>\n";
            }
            h << "</div>\n";
        }

        h << "</div>\n";
    }

    // ---- 页脚 ----
    h << "<div class=\"footer\">\n";
    h << "CALWF - Corpus-Anchored LLM Writing Forensics v1.0<br>\n";
    h << "语料锚定镜 · 基于 SWECCL 与 BAWE 的大模型英语写作取证系统\n";
    h << "</div>\n";

    h << "</body>\n</html>\n";
    return h.str();
}

bool ReportGenerator::saveHtml(
    const std::vector<analysis::AnalysisResult>& results,
    const std::string& title,
    const std::string& path) {
    std::string content = renderHtml(results, title);
    std::string dir = utils::dirName(path);
    if (!dir.empty()) utils::createDirs(dir);
    return utils::writeFile(path, content);
}

// ============================================================
// CSV
// ============================================================
std::string ReportGenerator::renderCsv(
    const std::vector<analysis::AnalysisResult>& results) {

    std::ostringstream csv;
    csv << "doc_id,source,ai_suspicion,human_likeness,"
        << "sweccl_anchoring,bawe_anchoring,verdict,risk_level,"
        << "top1_model,top1_confidence,"
        << "top2_model,top2_confidence,"
        << "top3_model,top3_confidence,"
        << "risk_word_hits,top_sweccl,top_bawe\n";

    char buf[1024];
    for (const auto& r : results) {
        std::string t1m, t2m, t3m;
        double t1c = 0, t2c = 0, t3c = 0;
        if (r.model_ranking.size() >= 1) {
            t1m = r.model_ranking[0].model_name;
            t1c = r.model_ranking[0].confidence;
        }
        if (r.model_ranking.size() >= 2) {
            t2m = r.model_ranking[1].model_name;
            t2c = r.model_ranking[1].confidence;
        }
        if (r.model_ranking.size() >= 3) {
            t3m = r.model_ranking[2].model_name;
            t3c = r.model_ranking[2].confidence;
        }

        std::string risk_hits;
        for (std::size_t i = 0; i < r.risk_hits.size() && i < 5; ++i) {
            if (i) risk_hits += "|";
            risk_hits += r.risk_hits[i];
        }
        std::string top_sw;
        for (std::size_t i = 0; i < r.top_sweccl_docs.size(); ++i) {
            if (i) top_sw += "|";
            top_sw += r.top_sweccl_docs[i];
        }
        std::string top_bw;
        for (std::size_t i = 0; i < r.top_bawe_docs.size(); ++i) {
            if (i) top_bw += "|";
            top_bw += r.top_bawe_docs[i];
        }

        std::snprintf(buf, sizeof(buf),
            "%s,%s,%.4f,%.4f,%.4f,%.4f,%s,%s,%s,%.4f,%s,%.4f,%s,%.4f,%s,%s,%s\n",
            csvEscape(r.doc_id).c_str(),
            csvEscape(r.source).c_str(),
            r.ai_suspicion,
            r.human_likeness,
            r.sweccl_anchoring,
            r.bawe_anchoring,
            csvEscape(r.verdict).c_str(),
            csvEscape(r.risk_level).c_str(),
            csvEscape(t1m).c_str(), t1c,
            csvEscape(t2m).c_str(), t2c,
            csvEscape(t3m).c_str(), t3c,
            csvEscape(risk_hits).c_str(),
            csvEscape(top_sw).c_str(),
            csvEscape(top_bw).c_str());
        csv << buf;
    }
    return csv.str();
}

bool ReportGenerator::saveCsv(
    const std::vector<analysis::AnalysisResult>& results,
    const std::string& path) {
    std::string content = renderCsv(results);
    std::string dir = utils::dirName(path);
    if (!dir.empty()) utils::createDirs(dir);
    return utils::writeFile(path, content);
}

} // namespace report
} // namespace calwf