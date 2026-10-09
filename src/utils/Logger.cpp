// ============================================================
// CALWF - Logger 实现
// ============================================================

#include "calwf/utils/Logger.h"

#include <iostream>
#include <ctime>
#include <cstring>
#include <vector>
#include <cstdio>

namespace calwf {
namespace utils {

// ------------------------------------------------------------
// 级别名
// ------------------------------------------------------------
const char* logLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "?????";
}

// ------------------------------------------------------------
// 单例
// ------------------------------------------------------------
Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() = default;

Logger::~Logger() {
    closeLogFile();
}

// ------------------------------------------------------------
// 文件
// ------------------------------------------------------------
bool Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    file_stream_.open(path, std::ios::out | std::ios::app);
    if (!file_stream_) {
        file_enabled_ = false;
        return false;
    }
    file_enabled_ = true;
    return true;
}

void Logger::closeLogFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    file_enabled_ = false;
}

// ------------------------------------------------------------
// 时间戳
// ------------------------------------------------------------
std::string Logger::timestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

// ------------------------------------------------------------
// 核心日志函数
// ------------------------------------------------------------
void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const std::string& message) {
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }

    std::string line_str;
    line_str.reserve(256);
    line_str += "[";
    line_str += timestamp();
    line_str += "] [";
    line_str += logLevelName(level);
    line_str += "] ";
    line_str += message;

    if (level == LogLevel::Debug) {
        line_str += "  (";
        line_str += file;
        line_str += ":";
        line_str += std::to_string(line);
        line_str += ")";
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (console_enabled_) {
        if (level == LogLevel::Error || level == LogLevel::Fatal) {
            std::cerr << line_str << std::endl;
        } else {
            std::cout << line_str << std::endl;
        }
    }

    if (file_enabled_ && file_stream_.is_open()) {
        file_stream_ << line_str << std::endl;
        file_stream_.flush();
    }
}

// ------------------------------------------------------------
// 格式化
// ------------------------------------------------------------
std::string Logger::format(const char* fmt, ...) {
    if (fmt == nullptr) return "";

    std::vector<char> buf(256);
    while (true) {
        va_list args;
        va_start(args, fmt);
        int n = std::vsnprintf(buf.data(), buf.size(), fmt, args);
        va_end(args);

        if (n < 0) {
            return std::string(fmt);
        }

        if (static_cast<std::size_t>(n) < buf.size()) {
            return std::string(buf.data(), static_cast<std::size_t>(n));
        }

        buf.resize(static_cast<std::size_t>(n) + 1);
    }
}

} // namespace utils
} // namespace calwf