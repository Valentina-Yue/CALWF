// ============================================================
// CALWF - Menu CLI 实现
// ============================================================

#include "calwf/cli/Menu.h"
#include "calwf/io/SwecclImporter.h"
#include "calwf/io/BaweImporter.h"
#include "calwf/io/LlmImporter.h"
#include "calwf/features/AnchoringFeatures.h"
#include "calwf/features/SafetyFeatures.h"
#include "calwf/model/ModelProfile.h"
#include "calwf/utils/FileUtils.h"
#include "calwf/utils/Logger.h"
#include "calwf/utils/StringUtils.h"
#include "calwf/text/SentenceSplitter.h"
#include "calwf/report/ReportGenerator.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <map>
#include <cstdio>
#include <chrono>


#ifdef _WIN32
    #include <windows.h>
#endif

namespace calwf {
namespace cli {

using namespace calwf::io;
using namespace calwf::model;
using namespace calwf::analysis;
using namespace calwf::features;

// ============================================================
// 路径
// ============================================================
std::string Menu::swecclPath() const {
    return raw_root_ + "/SWECCL/SWECCL/Raw Data (WECCL)";
}
std::string Menu::baweMetaPath() const {
    return data_root_ + "/metadata/bawe_metadata.csv";
}
std::string Menu::baweCsvPath() const {
    return raw_root_ + "/BAWE/bawe_csv_categorized/BAWE_csv_categorized";
}
std::string Menu::llmRootPath() const {
    return data_root_ + "/llm";
}
std::string Menu::llmMetaPath() const {
    return data_root_ + "/metadata/llm_meta.csv";
}
std::string Menu::riskWordsPath() const {
    return data_root_ + "/config/risk_words.txt";
}
std::string Menu::swecclCachePath() const {
    return data_root_ + "/index/sweccl.ngram";
}
std::string Menu::baweCachePath() const {
    return data_root_ + "/index/bawe.ngram";
}
std::string Menu::reportsDir() const {
    return data_root_ + "/reports";
}

// ============================================================
// 构造
// ============================================================
Menu::Menu() {
    // 默认路径
    data_root_ = "D:/CALWF/data";
    raw_root_  = "C:/Users/asus/Desktop/Linguistics/Corpus/Corpus sample";

    // 尝试读取 config.json
    std::string cfg = "D:/CALWF/config.json";
    if (utils::fileExists(cfg)) {
        std::string content = utils::readFile(cfg);
        // 简易 JSON 解析：找 "data_root" 和 "raw_root" 字段
        auto extract = [&](const std::string& key) -> std::string {
            std::string needle = "\"" + key + "\"";
            auto p = content.find(needle);
            if (p == std::string::npos) return "";
            p = content.find(':', p);
            if (p == std::string::npos) return "";
            auto q1 = content.find('"', p);
            if (q1 == std::string::npos) return "";
            auto q2 = content.find('"', q1 + 1);
            if (q2 == std::string::npos) return "";
            return content.substr(q1 + 1, q2 - q1 - 1);
        };
        std::string d = extract("data_root");
        std::string r = extract("raw_root");
        if (!d.empty()) data_root_ = d;
        if (!r.empty()) raw_root_  = r;
    }
}

// ============================================================
// 输入辅助
// ============================================================
std::string Menu::readLine() {
    std::string line;
    std::getline(std::cin, line);
    return utils::trim(line);
}

int Menu::readInt() {
    std::string line = readLine();
    if (line.empty()) return -1;
    return utils::toInt(line, -1);
}

void Menu::pause() {
    std::cout << "\n  按回车继续...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

// ============================================================
// 数据加载
// ============================================================
bool Menu::ensureSwecclLoaded() {
    if (sweccl_loaded_) return true;
    std::cout << "  正在加载 SWECCL...\n";
    SwecclImporter sw;
    sweccl_docs_ = sw.load(swecclPath());
    sweccl_loaded_ = true;
    std::cout << "  SWECCL 加载完成："
              << sweccl_docs_.size() << " 篇\n";
    return !sweccl_docs_.empty();
}

bool Menu::ensureBaweLoaded() {
    if (bawe_loaded_) return true;
    std::cout << "  正在加载 BAWE...\n";
    BaweImporter bw;
    bawe_docs_ = bw.load(baweMetaPath(), baweCsvPath());
    bawe_loaded_ = true;
    std::cout << "  BAWE 加载完成："
              << bawe_docs_.size() << " 篇\n";
    return !bawe_docs_.empty();
}

bool Menu::ensureLlmLoaded() {
    if (llm_loaded_) return true;
    std::cout << "  正在加载 LLM 输出...\n";
    LlmImporter lm;
    llm_docs_ = lm.load(llmRootPath(), llmMetaPath());
    llm_loaded_ = true;
    std::cout << "  LLM 加载完成："
              << llm_docs_.size() << " 篇\n";
    return !llm_docs_.empty();
}

bool Menu::ensureProfilesBuilt() {
    if (profiles_built_) return true;
    if (!ensureLlmLoaded()) return false;
    std::cout << "  构建模型指纹...\n";
    profiles_ = FingerprintBuilder::build(llm_docs_);
    profiles_built_ = true;
    return !profiles_.empty();
}

bool Menu::ensureAnalyzerReady() {
    // 1. 确保锚定器已初始化
    AnchoringFeatures::initialize(
        swecclPath(), baweMetaPath(), baweCsvPath(),
        swecclCachePath(), baweCachePath()
    );

    // 2. 确保风险词表已加载
    SafetyFeatures::initialize(riskWordsPath());

    // 3. 确保模型指纹
    if (!ensureProfilesBuilt()) return false;

    // 4. 确保参考语料（截断）
    ensureSwecclLoaded();
    ensureBaweLoaded();

    static bool analyzer_initialized = false;
    if (!analyzer_initialized) {
        analyzer_.setModelProfiles(profiles_);
        analyzer_.setAnchors(&AnchoringFeatures::swecclAnchor(),
                             &AnchoringFeatures::baweAnchor());

        // 截断参考语料
        if (sweccl_docs_.size() > REF_LIMIT) {
            sweccl_docs_.resize(REF_LIMIT);
            sweccl_docs_.shrink_to_fit();
        }
        if (bawe_docs_.size() > REF_LIMIT) {
            bawe_docs_.resize(REF_LIMIT);
            bawe_docs_.shrink_to_fit();
        }

        analyzer_.setReferenceCorpora(&sweccl_docs_, &bawe_docs_);
        analyzer_initialized = true;
    }

    return true;
}

// ============================================================
// 主循环
// ============================================================
void Menu::run() {
    // 初始化控制台为 UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    // 降低日志级别，避免刷屏
    utils::Logger::instance().setLevel(utils::LogLevel::Error);
    utils::Logger::instance().setConsoleOutput(false);

    while (true) {
        showMainMenu();
        int c = readInt();
        if (c == 0 || c == -1) {
            std::cout << "\n  再见！\n";
            break;
        }
        switch (c) {
            case 1: handleDataMenu();      break;
            case 2: handleProfilesMenu();  break;
            case 3: handleAnalysisMenu();  break;
            case 4: handleReportMenu();    break;
            default:
                std::cout << "  无效选择\n";
        }
    }
}

// ============================================================
// 主菜单
// ============================================================
void Menu::showMainMenu() {
    std::cout << "\n";
    std::cout << "  ╔════════════════════════════════════════╗\n";
    std::cout << "  ║  CALWF 语料锚定镜                       ║\n";
    std::cout << "  ║  Corpus-Anchored LLM Writing Forensics  ║\n";
    std::cout << "  ╚════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "    1. 数据管理\n";
    std::cout << "    2. 模型指纹\n";
    std::cout << "    3. 文本分析\n";
    std::cout << "    4. 报告导出\n";
    std::cout << "\n";
    std::cout << "    0. 退出\n";
    std::cout << "\n";
    std::cout << "  请选择：";
}

// ============================================================
// 数据管理
// ============================================================
void Menu::handleDataMenu() {
    while (true) {
        std::cout << "\n";
        std::cout << "  ┌─ 数据管理 ─────────────────────────┐\n";
        std::cout << "  │  1. 查看数据统计                    │\n";
        std::cout << "  │  2. 重新加载所有数据                │\n";
        std::cout << "  │  0. 返回                            │\n";
        std::cout << "  └─────────────────────────────────────┘\n";
        std::cout << "  请选择：";

        int c = readInt();
        if (c == 0 || c == -1) return;
        switch (c) {
            case 1: showDataStats(); break;
            case 2: reloadAllData(); break;
            default: std::cout << "  无效选择\n";
        }
    }
}

void Menu::showDataStats() {
    std::cout << "\n  ───────── 数据统计 ─────────\n";

    // SWECCL
    if (sweccl_loaded_) {
        int arg = 0, exp = 0, nar = 0, other = 0;
        for (const auto& d : sweccl_docs_) {
            if (d.genre == "ARG") ++arg;
            else if (d.genre == "EXP") ++exp;
            else if (d.genre == "NAR") ++nar;
            else ++other;
        }
        std::cout << "  SWECCL: " << sweccl_docs_.size() << " 篇"
                  << "  (ARG=" << arg << ", EXP=" << exp
                  << ", NAR=" << nar << ")\n";
    } else {
        std::cout << "  SWECCL: 未加载\n";
    }

    // BAWE
    if (bawe_loaded_) {
        std::cout << "  BAWE:   " << bawe_docs_.size() << " 篇\n";
    } else {
        std::cout << "  BAWE:   未加载\n";
    }

    // LLM
    if (llm_loaded_) {
        std::cout << "  LLM:    " << llm_docs_.size() << " 篇";
        if (profiles_built_) {
            std::cout << "  (模型指纹已构建，"
                      << profiles_.size() << " 个模型)";
        }
        std::cout << "\n";
    } else {
        std::cout << "  LLM:    未加载\n";
    }

    pause();
}

void Menu::reloadAllData() {
    std::cout << "\n  确认重新加载所有数据？(y/n)：";
    std::string ans = utils::toLower(readLine());
    if (ans != "y" && ans != "yes") {
        std::cout << "  已取消\n";
        return;
    }

    sweccl_docs_.clear();
    bawe_docs_.clear();
    llm_docs_.clear();
    profiles_.clear();
    sweccl_loaded_ = false;
    bawe_loaded_ = false;
    llm_loaded_ = false;
    profiles_built_ = false;

    std::cout << "  已清空。下次使用时会重新加载。\n";
    pause();
}

// ============================================================
// 模型指纹
// ============================================================
void Menu::handleProfilesMenu() {
    while (true) {
        std::cout << "\n";
        std::cout << "  ┌─ 模型指纹 ─────────────────────────┐\n";
        std::cout << "  │  1. 查看概览                        │\n";
        std::cout << "  │  2. 查看某个模型详情                │\n";
        std::cout << "  │  0. 返回                            │\n";
        std::cout << "  └─────────────────────────────────────┘\n";
        std::cout << "  请选择：";

        int c = readInt();
        if (c == 0 || c == -1) return;
        switch (c) {
            case 1: showProfilesOverview(); break;
            case 2: showProfileDetail();    break;
            default: std::cout << "  无效选择\n";
        }
    }
}

void Menu::showProfilesOverview() {
    if (!ensureProfilesBuilt()) {
        std::cout << "  构建失败\n";
        pause();
        return;
    }
    FingerprintBuilder::print(profiles_);
    pause();
}

void Menu::showProfileDetail() {
    if (!ensureProfilesBuilt()) return;

    std::cout << "\n  可用模型：\n";
    for (std::size_t i = 0; i < profiles_.size(); ++i) {
        std::cout << "    " << (i + 1) << ". "
                  << profiles_[i].model_name << "\n";
    }
    std::cout << "  请选择：";

    int c = readInt();
    if (c < 1 || static_cast<std::size_t>(c) > profiles_.size()) {
        std::cout << "  无效选择\n";
        return;
    }

    const auto& p = profiles_[static_cast<std::size_t>(c - 1)];
    std::cout << "\n  ───── " << p.model_name << " 特征详情 ─────\n";
    std::cout << "  样本数: " << p.sample_count << "\n\n";
    p.mean.print();
    pause();
}

// ============================================================
// 文本分析
// ============================================================
void Menu::handleAnalysisMenu() {
    while (true) {
        std::cout << "\n";
        std::cout << "  ┌─ 文本分析 ─────────────────────────┐\n";
        std::cout << "  │  1. 分析 LLM 文档（按模型/prompt）  │\n";
        std::cout << "  │  2. 分析自定义文件                  │\n";
        std::cout << "  │  3. 粘贴文本分析（教师模式）        │\n";
        std::cout << "  │  4. 批量分析                        │\n";
        std::cout << "  │  0. 返回                            │\n";
        std::cout << "  └─────────────────────────────────────┘\n";
        std::cout << "  请选择：";

        int c = readInt();
        if (c == 0 || c == -1) return;
        switch (c) {
            case 1: analyzeLlmDoc();      break;
            case 2: analyzeFromFile();    break;
            case 3: analyzeFromPaste();   break;
            case 4: batchAnalyze();       break;
            default: std::cout << "  无效选择\n";
        }
    }
}

void Menu::analyzeLlmDoc() {
    if (!ensureAnalyzerReady()) {
        std::cout << "  初始化失败\n";
        pause();
        return;
    }

    std::cout << "\n  可选模型：\n";
    // 去重列出模型名
    std::vector<std::string> models;
    for (const auto& d : llm_docs_) {
        if (std::find(models.begin(), models.end(), d.model_name)
            == models.end()) {
            models.push_back(d.model_name);
        }
    }
    for (std::size_t i = 0; i < models.size(); ++i) {
        std::cout << "    " << (i + 1) << ". " << models[i] << "\n";
    }
    std::cout << "  请选择模型：";

    int mc = readInt();
    if (mc < 1 || static_cast<std::size_t>(mc) > models.size()) {
        std::cout << "  无效选择\n";
        return;
    }
    std::string model = models[static_cast<std::size_t>(mc - 1)];

    std::cout << "\n  可选 prompt：\n";
    std::vector<std::string> prompts;
    for (const auto& d : llm_docs_) {
        if (d.model_name != model) continue;
        if (std::find(prompts.begin(), prompts.end(), d.prompt_id)
            == prompts.end()) {
            prompts.push_back(d.prompt_id);
        }
    }
    std::sort(prompts.begin(), prompts.end());
    for (std::size_t i = 0; i < prompts.size(); ++i) {
        std::cout << "    " << (i + 1) << ". " << prompts[i] << "\n";
    }
    std::cout << "  请选择 prompt：";

    int pc = readInt();
    if (pc < 1 || static_cast<std::size_t>(pc) > prompts.size()) {
        std::cout << "  无效选择\n";
        return;
    }
    std::string prompt = prompts[static_cast<std::size_t>(pc - 1)];

    std::cout << "\n  可选 run：1, 2, 3\n  请选择：";
    int rc = readInt();
    if (rc < 1 || rc > 3) {
        std::cout << "  无效选择\n";
        return;
    }

    // 找对应文档
    for (const auto& d : llm_docs_) {
        if (d.model_name == model &&
            d.prompt_id == prompt &&
            d.run_number == rc) {
            auto r = analyzer_.analyze(d);
            printReport(r, model + " / " + prompt + " / run" + std::to_string(rc));
            pause();
            return;
        }
    }
    std::cout << "  未找到对应文档\n";
    pause();
}

void Menu::analyzeFromPaste() {
    if (!ensureAnalyzerReady()) return;

    std::cout << "\n";
    std::cout << "  ═══════════════════════════════════════\n";
    std::cout << "    教师模式：粘贴学生作文进行 AI 检测\n";
    std::cout << "  ═══════════════════════════════════════\n";
    std::cout << "\n";
    std::cout << "  操作说明：\n";
    std::cout << "    1. 粘贴作文内容（可多行）\n";
    std::cout << "    2. 输入 END（单独一行，大写）结束输入\n";
    std::cout << "    3. 输入 CANCEL 取消\n";
    std::cout << "\n";
    std::cout << "  请输入作文内容：\n";
    std::cout << "  ─────────────────────────────────────\n";

    std::string content;
    std::string line;
    while (true) {
        if (!std::getline(std::cin, line)) break;

        std::string trimmed = utils::trim(line);
        if (trimmed == "END")   break;
        if (trimmed == "CANCEL") {
            std::cout << "  已取消\n";
            return;
        }
        content += line;
        content += '\n';
    }

    if (utils::trim(content).empty()) {
        std::cout << "  内容为空，已取消\n";
        return;
    }

    // 统计
    std::cout << "  ─────────────────────────────────────\n";
    std::cout << "  输入完成：共 " << content.size() << " 字符\n";

    // 构造临时 Document
    Document doc;
    doc.doc_id = "PASTE_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    doc.source = SourceType::Unknown;

    auto paragraphs = calwf::text::SentenceSplitter::splitParagraphs(content);
    int para_id = 0;
    for (auto& sents : paragraphs) {
        Paragraph p;
        p.id = ++para_id;
        std::string ptext;
        int sent_id = 0;
        for (auto& s : sents) {
            Sentence sent;
            sent.id = ++sent_id;
            sent.text = s;

            std::string word;
            int wid = 0;
            for (char ch : s) {
                if (ch == ' ' || ch == '\t') {
                    if (!word.empty()) {
                        Token t;
                        t.sentence_id = sent.id;
                        t.word_id = ++wid;
                        t.word = word;
                        sent.tokens.push_back(t);
                        word.clear();
                    }
                } else {
                    word.push_back(ch);
                }
            }
            if (!word.empty()) {
                Token t;
                t.sentence_id = sent.id;
                t.word_id = ++wid;
                t.word = word;
                sent.tokens.push_back(t);
            }

            if (!sent.text.empty()) {
                if (!ptext.empty()) ptext += ' ';
                ptext += sent.text;
                p.sentences.push_back(sent);
            }
        }
        p.text = ptext;
        if (!p.empty()) doc.paragraphs.push_back(std::move(p));
    }

    doc.recomputeStats();

    // 分析
    auto r = analyzer_.analyze(doc);
    printReport(r, "粘贴文本分析");

    // 简单结论
    std::cout << "\n";
    std::cout << "  ═══════════════════════════════════════\n";
    std::cout << "    结论\n";
    std::cout << "  ═══════════════════════════════════════\n";
    std::cout << "  词数: " << doc.word_count << "\n";
    std::cout << "  句数: " << doc.sentence_count << "\n";
    std::cout << "  段数: " << doc.paragraph_count << "\n";

    if (r.ai_suspicion > 0.60) {
        std::cout << "  ⚠ 该作文高度疑似 AI 生成（嫌疑度 "
                  << static_cast<int>(r.ai_suspicion * 100) << "%）\n";
        std::cout << "  建议：人工复核，并参考下方证据链\n";
    } else if (r.ai_suspicion < 0.35) {
        std::cout << "  ✓ 该作文大概率是真人写作（人味指数 "
                  << static_cast<int>(r.human_likeness * 100) << "%）\n";
    } else {
        std::cout << "  ? 该作文介于真人和 AI 之间，需要人工判断\n";
    }

    pause();
}

void Menu::analyzeFromFile() {
    if (!ensureAnalyzerReady()) return;

    std::cout << "\n  请输入文件路径：";
    std::string path = readLine();
    if (path.empty()) {
        std::cout << "  路径为空\n";
        return;
    }
    if (!utils::fileExists(path)) {
        std::cout << "  文件不存在：" << path << "\n";
        return;
    }

    std::string content = utils::readFile(path);
    if (content.empty()) {
        std::cout << "  文件为空\n";
        return;
    }

    // 构造临时 Document
    Document doc;
    doc.doc_id = "FILE_" + utils::fileNameNoExt(path);
    doc.source = SourceType::Unknown;

    auto paragraphs = calwf::text::SentenceSplitter::splitParagraphs(content);
    int para_id = 0;
    for (auto& sents : paragraphs) {
        Paragraph p;
        p.id = ++para_id;
        std::string ptext;
        int sent_id = 0;
        for (auto& s : sents) {
            Sentence sent;
            sent.id = ++sent_id;
            sent.text = s;

            std::string word;
            int wid = 0;
            for (char ch : s) {
                if (ch == ' ' || ch == '\t') {
                    if (!word.empty()) {
                        Token t;
                        t.sentence_id = sent.id;
                        t.word_id = ++wid;
                        t.word = word;
                        sent.tokens.push_back(t);
                        word.clear();
                    }
                } else {
                    word.push_back(ch);
                }
            }
            if (!word.empty()) {
                Token t;
                t.sentence_id = sent.id;
                t.word_id = ++wid;
                t.word = word;
                sent.tokens.push_back(t);
            }

            if (!sent.text.empty()) {
                if (!ptext.empty()) ptext += ' ';
                ptext += sent.text;
                p.sentences.push_back(sent);
            }
        }
        p.text = ptext;
        if (!p.empty()) doc.paragraphs.push_back(std::move(p));
    }

    doc.recomputeStats();

    auto r = analyzer_.analyze(doc);
    printReport(r, "自定义文件：" + utils::fileName(path));
    pause();
}

void Menu::batchAnalyze() {
    if (!ensureAnalyzerReady()) return;

    std::cout << "\n  对全部 " << llm_docs_.size()
              << " 篇 LLM 文档做批量分析？(y/n)：";
    std::string ans = utils::toLower(readLine());
    if (ans != "y" && ans != "yes") {
        std::cout << "  已取消\n";
        return;
    }

    // 按模型统计
    struct Stat {
        int    count = 0;
        int    ai_cnt = 0;
        double sum_sus = 0.0;
    };
    std::map<std::string, Stat> stats;

    for (const auto& d : llm_docs_) {
        auto r = analyzer_.analyze(d);
        auto& s = stats[d.model_name];
        ++s.count;
        s.sum_sus += r.ai_suspicion;
        if (r.verdict == "AI") ++s.ai_cnt;
    }

    std::cout << "\n  ───── 批量分析结果 ─────\n";
    std::cout << "  " << std::string(10, ' ')
              << " | 篇数  判AI  平均嫌疑度\n";
    std::cout << "  " << std::string(50, '-') << "\n";
    for (const auto& kv : stats) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "  %-10s | %4d  %4d  %8.2f %%",
            kv.first.c_str(),
            kv.second.count,
            kv.second.ai_cnt,
            100.0 * kv.second.sum_sus / kv.second.count);
        std::cout << buf << "\n";
    }
    pause();
}

// ============================================================
// 报告
// ============================================================
void Menu::handleReportMenu() {
    while (true) {
        std::cout << "\n";
        std::cout << "  ┌─ 报告导出 ─────────────────────────┐\n";
        std::cout << "  │  1. 导出 Markdown 报告              │\n";
        std::cout << "  │  0. 返回                            │\n";
        std::cout << "  └─────────────────────────────────────┘\n";
        std::cout << "  请选择：";

        int c = readInt();
        if (c == 0 || c == -1) return;
        switch (c) {
            case 1: exportMarkdownReport(); break;
            default: std::cout << "  无效选择\n";
        }
    }
}

void Menu::exportMarkdownReport() {
    if (!ensureAnalyzerReady()) return;

    std::cout << "\n  为哪些文档生成报告？\n";
    std::cout << "    1. 全部 LLM 文档（180 篇）\n";
    std::cout << "    2. 只 p001_run1（5 篇）\n";
    std::cout << "    0. 取消\n";
    std::cout << "  请选择：";

    int c = readInt();
    if (c == 0 || c == -1) return;

    std::vector<const Document*> targets;
    if (c == 1) {
        for (const auto& d : llm_docs_) targets.push_back(&d);
    } else if (c == 2) {
        for (const auto& d : llm_docs_) {
            if (d.prompt_id == "p001" && d.run_number == 1) {
                targets.push_back(&d);
            }
        }
    } else {
        std::cout << "  无效选择\n";
        return;
    }

    if (targets.empty()) {
        std::cout << "  无匹配文档\n";
        return;
    }

    // 收集结果
    std::vector<analysis::AnalysisResult> results;
    for (const auto* dp : targets) {
        results.push_back(analyzer_.analyze(*dp));
    }

    // 生成三个格式
    utils::createDirs(reportsDir());

    std::string base = reportsDir() + "/report_" +
                       std::to_string(targets.size()) + "docs";

    std::string md_path  = base + ".md";
    std::string html_path = base + ".html";
    std::string csv_path  = base + ".csv";

    std::string title = "CALWF 分析报告";

    bool ok1 = report::ReportGenerator::saveMarkdown(results, title, md_path);
    bool ok2 = report::ReportGenerator::saveHtml(results, title, html_path);
    bool ok3 = report::ReportGenerator::saveCsv(results, csv_path);

    std::cout << "\n  报告已生成：\n";
    std::cout << "    " << (ok1 ? "[OK] " : "[FAIL] ") << md_path   << "\n";
    std::cout << "    " << (ok2 ? "[OK] " : "[FAIL] ") << html_path << "\n";
    std::cout << "    " << (ok3 ? "[OK] " : "[FAIL] ") << csv_path  << "\n";
    std::cout << "\n  用浏览器打开 HTML 查看可视化报告。\n";
    pause();
}

// ============================================================
// 打印单份报告
// ============================================================
void Menu::printReport(const AnalysisResult& r,
                       const std::string& title) {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════╗\n";
    std::cout << "  ║ " << title << "\n";
    std::cout << "  ╚══════════════════════════════════════════════════╝\n";
    std::cout << "  文档 ID    : " << r.doc_id << "\n";
    std::cout << "  数据来源   : " << r.source << "\n";
    std::cout << "  ─────────────────────────────────────\n";

    char buf[256];
    std::snprintf(buf, sizeof(buf), "  AI 嫌疑度  : %.2f %%\n",
                  100.0 * r.ai_suspicion);
    std::cout << buf;
    std::snprintf(buf, sizeof(buf), "  人味指数   : %.2f %%\n",
                  100.0 * r.human_likeness);
    std::cout << buf;
    std::snprintf(buf, sizeof(buf), "  SWECCL 锚定: %.2f %%\n",
                  100.0 * r.sweccl_anchoring);
    std::cout << buf;
    std::snprintf(buf, sizeof(buf), "  BAWE   锚定: %.2f %%\n",
                  100.0 * r.bawe_anchoring);
    std::cout << buf;

    std::cout << "  判定       : " << r.verdict << "\n";
    std::cout << "  风险等级   : " << r.risk_level << "\n";

    if (!r.model_ranking.empty()) {
        std::cout << "  ─────────────────────────────────────\n";
        std::cout << "  模型归因（top-3）:\n";
        for (const auto& m : r.model_ranking) {
            std::snprintf(buf, sizeof(buf),
                "    %-10s  距离 %6.2f  置信度 %5.2f %%\n",
                m.model_name.c_str(), m.distance,
                100.0 * m.confidence);
            std::cout << buf;
        }
    }

    if (!r.risk_reasons.empty()) {
        std::cout << "  ─────────────────────────────────────\n";
        std::cout << "  风险原因:\n";
        for (const auto& s : r.risk_reasons) {
            std::cout << "    - " << s << "\n";
        }
    }

    if (!r.risk_hits.empty()) {
        std::cout << "  ─────────────────────────────────────\n";
        std::cout << "  风险词命中（前 5）:\n";
        std::size_t n = std::min<std::size_t>(5, r.risk_hits.size());
        for (std::size_t i = 0; i < n; ++i) {
            std::cout << "    - " << r.risk_hits[i] << "\n";
        }
    }

    if (!r.top_sweccl_docs.empty()) {
        std::cout << "  ─────────────────────────────────────\n";
        std::cout << "  最相似 SWECCL 文档:\n";
        for (const auto& s : r.top_sweccl_docs) {
            std::cout << "    - " << s << "\n";
        }
    }
    if (!r.top_bawe_docs.empty()) {
        std::cout << "  最相似 BAWE 文档:\n";
        for (const auto& s : r.top_bawe_docs) {
            std::cout << "    - " << s << "\n";
        }
    }
}

} // namespace cli
} // namespace calwf