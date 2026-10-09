// ============================================================
// CALWF - FileUtils 实现
// ============================================================

#include "calwf/utils/FileUtils.h"
#include "calwf/utils/StringUtils.h"

#include <algorithm>      // ← 新增，用于 std::sort
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace calwf {
namespace utils {

// ============ 文件读写 ============

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path, std::ios::binary);
    if (!in) return lines;

    std::string line;
    while (std::getline(in, line)) {
        // 去掉行尾的 \r（Windows 换行）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return out.good();
}

bool writeLines(const std::string& path,
                const std::vector<std::string>& lines) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    for (const auto& line : lines) {
        out << line << '\n';
    }
    return out.good();
}

bool appendFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) return false;
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return out.good();
}

// ============ 文件系统 ============

bool fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool dirExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

bool createDir(const std::string& path) {
    std::error_code ec;
    return fs::create_directory(path, ec) || fs::exists(path);
}

bool createDirs(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    return fs::exists(path);
}

std::vector<std::string> listFiles(const std::string& dir,
                                   const std::string& extension) {
    std::vector<std::string> result;
    if (!dirExists(dir)) return result;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (extension.empty() ||
            fileExtension(path) == extension) {
            result.push_back(path);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> listFilesRecursive(const std::string& dir,
                                            const std::string& extension) {
    std::vector<std::string> result;
    if (!dirExists(dir)) return result;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (extension.empty() ||
            fileExtension(path) == extension) {
            result.push_back(path);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

// ============ 路径处理 ============

std::string fileName(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string fileNameNoExt(const std::string& path) {
    return fs::path(path).stem().string();
}

std::string fileExtension(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    // 去掉前导点
    if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
    return ext;
}

std::string dirName(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    char last = a.back();
    if (last == '/' || last == '\\') {
        return a + b;
    }
    return a + "/" + b;
}

} // namespace utils
} // namespace calwf