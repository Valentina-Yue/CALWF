# ============================================================
# CALWF - Corpus-Anchored LLM Writing Forensics
# 顶层 CMakeLists.txt
# ============================================================
#
# 项目：语料锚定镜——基于 SWECCL 与 BAWE 的
#      大模型英语写作风格取证与安全审计系统
#
# 构建系统：CMake 3.16+
# 语言标准：C++17
# 主编译器：MinGW g++ 13.1.0（Qt 6.10.2 自带）
#
# 用法：
#   cmake -S . -B build -G "MinGW Makefiles"
#   cmake --build build
# ============================================================

# ---------- 最低 CMake 版本 ----------
cmake_minimum_required(VERSION 3.16)

# ---------- 项目信息 ----------
project(CALWF
    VERSION 1.0.0
    DESCRIPTION "Corpus-Anchored LLM Writing Forensics"
    LANGUAGES CXX
)

# ---------- 语言标准 ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- 导出 compile_commands.json（给 IDE / clangd 用） ----------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------- 构建类型 ----------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release"
        CACHE STRING "Build type: Debug, Release, RelWithDebInfo" FORCE)
endif()

# ---------- 输出目录 ----------
# 所有可执行文件和库都统一输出到 build/bin，方便运行
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------- 编译选项 ----------
if(MSVC)
    # MSVC 分支（本项目当前不用，但保留兼容）
    add_compile_options(/W4 /utf-8)
else()
    # GCC / MinGW 分支
    add_compile_options(
        -Wall              # 常用警告
        -Wextra            # 额外警告
        -Wpedantic         # 严格标准符合性
        -Wshadow           # 变量遮蔽
        -Wnon-virtual-dtor # 有虚函数但析构非虚
        -Wold-style-cast   # 禁止 C 风格强制转换
        -Wcast-align       # 指针转换对齐
        -Wunused           # 未使用
        -Woverloaded-virtual # 虚函数重载
        -Wconversion       # 隐式类型转换
        -Wsign-conversion  # 符号转换
        -Wdouble-promotion # float 隐式提升 double
        -Wformat=2         # printf 格式检查
    )
endif()

# ---------- 静态链接（关键：让 exe 不依赖 MinGW DLL）----------
# 目的：让 calwf_cli.exe 可以直接双击或在任意 PowerShell 里运行，
#       不再依赖 libstdc++-6.dll、libgcc_s_seh-1.dll 等。
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

# ---------- 全局 include 目录 ----------
# 让所有 target 都能 #include "calwf/..."
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---------- 子目录 ----------
# 核心库
add_subdirectory(src)

# 测试（如果存在）
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
endif()

# GUI（可选，用 -DBUILD_GUI=ON 打开）
option(BUILD_GUI "Build Qt GUI" OFF)
if(BUILD_GUI)
    add_subdirectory(gui)
endif()

# ---------- 打印配置摘要 ----------
message(STATUS "")
message(STATUS "=================================================")
message(STATUS " CALWF 配置摘要")
message(STATUS "=================================================")
message(STATUS " 项目版本    : ${PROJECT_VERSION}")
message(STATUS " 构建类型    : ${CMAKE_BUILD_TYPE}")
message(STATUS " C++ 标准    : ${CMAKE_CXX_STANDARD}")
message(STATUS " 源代码目录  : ${CMAKE_SOURCE_DIR}")
message(STATUS " 构建目录    : ${CMAKE_BINARY_DIR}")
message(STATUS " 可执行输出  : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS " GUI 构建    : ${BUILD_GUI}")
message(STATUS "=================================================")
message(STATUS "")