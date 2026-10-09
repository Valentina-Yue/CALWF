// ============================================================
// CALWF - Menu CLI 菜单
// ============================================================
//
// 文件名：include/calwf/cli/Menu.h
// 作用：交互式命令行界面。
//
// 架构：
//   Menu 类持有所有共享状态（语料库、锚定器、模型指纹等），
//   各子菜单是私有方法，主循环 run() 负责调度。
//
// 懒加载：
//   - 启动时只加载锚定器（从缓存）
//   - SWECCL / BAWE / LLM 数据在首次使用时加载
//   - 模型指纹在首次分析时构建
//
// 交互：
//   - 数字选择菜单项
//   - 输入 "0" 或 "b" 返回上一级
//   - 输入 "q" 退出程序
// ============================================================

#ifndef CALWF_CLI_MENU_H
#define CALWF_CLI_MENU_H

#include <string>
#include <vector>

#include "calwf/core/Corpus.h"
#include "calwf/features/CorpusAnchor.h"
#include "calwf/model/ModelProfile.h"
#include "calwf/analysis/Analyzer.h"
#include "calwf/analysis/AnalysisResult.h"

namespace calwf {
namespace cli {

class Menu {
public:
    Menu();

    // 运行主循环
    void run();

private:
    // 数据根目录（从 config.json 读取，默认为程序目录下的 data/）
    std::string data_root_;
    std::string raw_root_;
    
    // ============ 状态 ============
    bool sweccl_loaded_ = false;
    bool bawe_loaded_   = false;
    bool llm_loaded_    = false;
    bool profiles_built_ = false;

    std::vector<Document> sweccl_docs_;
    std::vector<Document> bawe_docs_;
    std::vector<Document> llm_docs_;

    std::vector<model::ModelProfile> profiles_;

    analysis::Analyzer analyzer_;

    // 参考语料只保留前 N 篇（避免 OOM）
    static constexpr std::size_t REF_LIMIT = 100;

    // ============ 输入辅助 ============
    // 读一行，返回去除首尾空白的字符串
    static std::string readLine();

    // 读一个整数，返回 -1 表示输入无效
    static int readInt();

    // 等待按回车
    static void pause();

    // ============ 数据加载 ============
    bool ensureSwecclLoaded();
    bool ensureBaweLoaded();
    bool ensureLlmLoaded();
    bool ensureProfilesBuilt();
    bool ensureAnalyzerReady();

    // ============ 主菜单 ============
    void showMainMenu();
    void handleDataMenu();
    void handleProfilesMenu();
    void handleAnalysisMenu();
    void handleReportMenu();

    // ============ 子菜单实现 ============
    // 数据管理
    void showDataStats();
    void reloadAllData();

    // 模型指纹
    void showProfilesOverview();
    void showProfileDetail();

    // 文本分析
    void analyzeLlmDoc();
    void analyzeFromFile();
    void analyzeFromPaste();
    void batchAnalyze();

    // 报告
    void exportMarkdownReport();

    // ============ 工具 ============
    void printReport(const analysis::AnalysisResult& r,
                     const std::string& title);

    // 路径常量
    std::string swecclPath() const;
    std::string baweMetaPath() const;
    std::string baweCsvPath() const;
    std::string llmRootPath() const;
    std::string llmMetaPath() const;
    std::string riskWordsPath() const;
    std::string swecclCachePath() const;
    std::string baweCachePath() const;
    std::string reportsDir() const;
};

} // namespace cli
} // namespace calwf

#endif // CALWF_CLI_MENU_H