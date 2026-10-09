// ============================================================
// CALWF - 版本信息实现
// ============================================================
//
// 文件名：src/core/calwf_version.cpp
// 作用：提供 calwf_core 库的版本信息。
//       目前只作为 calwf_core 的占位源文件，
//       防止 CMake 报 "No SOURCES given to target"。
//
// 后续会加入更多核心模块的 .cpp。
// ============================================================

#include <string>

namespace calwf {

// 库版本号（与顶层 CMakeLists.txt 保持一致）
const char* version() {
    return "1.0.0";
}

// 项目名称
const char* projectName() {
    return "CALWF (Corpus-Anchored LLM Writing Forensics)";
}

// 项目中文名
const char* projectNameZh() {
    return "语料锚定镜";
}

} // namespace calwf