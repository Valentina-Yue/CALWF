// ============================================================
// CALWF - Logger 日志系统
// ============================================================
//
// 文件名：include/calwf/utils/Logger.h
// 作用：统一的日志输出，支持控制台 + 文件，带级别和时间戳。
//
// 用法：
//   auto& logger = Logger::instance();
//   logger.setLevel(LogLevel::Debug);
//   logger.setLogFile("D:/CALWF/data/reports/calwf.log");
//   CALWF_LOG_INFO("已导入 %d 篇文档", n);
//
// 注意：
//   枚举值用首字母大写（Debug/Info/Warn/Error/Fatal），
//   避免与 windows.h 中的 ERROR 宏冲突。
//
// 对应数据结构课程知识：类 / 封装 / 单例模式
// ============================================================

#ifndef CALWF_UTILS_LOGGER_H
#define CALWF_UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <cstdarg>

namespace calwf {
namespace utils {

// ------------------------------------------------------------
// 日志级别
// ------------------------------------------------------------
// 使用首字母大写，避免与 windows.h 中定义的 ERROR、FATAL 等宏冲突。
// ------------------------------------------------------------
enum class LogLevel {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    Fatal = 4
};

const char* logLevelName(LogLevel level);

// ------------------------------------------------------------
// Logger 单例
// ------------------------------------------------------------
class Logger {
public:
    static Logger& instance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void setLevel(LogLevel level) { level_ = level; }
    LogLevel level() const { return level_; }

    bool setLogFile(const std::string& path);
    void closeLogFile();

    void setConsoleOutput(bool enabled) { console_enabled_ = enabled; }
    bool consoleOutput() const { return console_enabled_; }

    void setFileOutput(bool enabled) { file_enabled_ = enabled; }
    bool fileOutput() const { return file_enabled_; }

    void log(LogLevel level,
             const char* file,
             int line,
             const std::string& message);

    static std::string format(const char* fmt, ...);

private:
    Logger();
    ~Logger();

    LogLevel      level_ = LogLevel::Info;
    bool          console_enabled_ = true;
    bool          file_enabled_    = false;
    std::ofstream file_stream_;
    std::mutex    mutex_;

    static std::string timestamp();
};

} // namespace utils
} // namespace calwf

// ============================================================
// 便捷宏
// ============================================================
// 全部使用 (...) 形式，不需要 ##__VA_ARGS__，
// 兼容 ISO C++，不触发 -Wpedantic 警告。
// ============================================================

#define CALWF_LOG_DEBUG(...) \
    ::calwf::utils::Logger::instance().log( \
        ::calwf::utils::LogLevel::Debug, __FILE__, __LINE__, \
        ::calwf::utils::Logger::format(__VA_ARGS__))

#define CALWF_LOG_INFO(...) \
    ::calwf::utils::Logger::instance().log( \
        ::calwf::utils::LogLevel::Info, __FILE__, __LINE__, \
        ::calwf::utils::Logger::format(__VA_ARGS__))

#define CALWF_LOG_WARN(...) \
    ::calwf::utils::Logger::instance().log( \
        ::calwf::utils::LogLevel::Warn, __FILE__, __LINE__, \
        ::calwf::utils::Logger::format(__VA_ARGS__))

#define CALWF_LOG_ERROR(...) \
    ::calwf::utils::Logger::instance().log( \
        ::calwf::utils::LogLevel::Error, __FILE__, __LINE__, \
        ::calwf::utils::Logger::format(__VA_ARGS__))

#define CALWF_LOG_FATAL(...) \
    ::calwf::utils::Logger::instance().log( \
        ::calwf::utils::LogLevel::Fatal, __FILE__, __LINE__, \
        ::calwf::utils::Logger::format(__VA_ARGS__))

#endif // CALWF_UTILS_LOGGER_H