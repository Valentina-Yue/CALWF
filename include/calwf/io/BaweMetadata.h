// ============================================================
// CALWF - BAWE 元数据结构
// ============================================================
//
// 文件名：include/calwf/io/BaweMetadata.h
// 作用：定义 BAWE 元数据表的行结构，并提供
//       从 CSV 行到结构的转换。
//
// BAWE 元数据 CSV 的 27 列（实际使用其中若干列）：
//   level
//   disciplinary group
//   genre family
//   discipline
//   L1
//   title
//   module
//   course
//   education
//   student
//   id            ← 主键，与 CSV 文件名对应
//   date
//   grade
//   words
//   s-units
//   p-units
//   tables
//   figures
//   block quotes
//   formulae
//   lists
//   listlikes
//   abstract
//   w/s
//   s/p
//   gender
//   year of birth
//
// 对应数据结构课程知识：类 / 封装 / 哈希表（id → 元数据）
// ============================================================

#ifndef CALWF_IO_BAWEMETADATA_H
#define CALWF_IO_BAWEMETADATA_H

#include <string>
#include <vector>
#include <unordered_map>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 单条元数据
// ------------------------------------------------------------
struct BaweMetadataEntry {
    int         level               = -1;    // 1-4
    std::string disciplinary_group;          // AH / LS / PS / SS
    std::string genre_family;                // essay / critique / ...
    std::string discipline;                  // Agriculture / Psychology / ...
    std::string l1;                          // 母语
    std::string title;                       // 题目
    std::string module;
    std::string course;
    std::string education;                   // UKa / OSa / UK1 ...
    std::string student;                     // 学号
    std::string id;                          // 文档 ID，如 6008h
    std::string date;
    std::string grade;                       // M / D
    int         words               = 0;
    int         s_units             = 0;
    int         p_units             = 0;
    std::string gender;                      // m / f
    int         year_of_birth       = -1;
};

// ------------------------------------------------------------
// 元数据表（id → 元数据）
// ------------------------------------------------------------
class BaweMetadataTable {
public:
    // 从 CSV 文件加载
    // 返回是否成功
    bool loadFromFile(const std::string& path);

    // 按 id 查找（未找到返回 nullptr）
    const BaweMetadataEntry* find(const std::string& id) const;

    // 数据条数
    std::size_t size() const {
        return table_.size();
    }

    // 全部 id（用于调试）
    std::vector<std::string> allIds() const;

private:
    std::unordered_map<std::string, BaweMetadataEntry> table_;
};

} // namespace io
} // namespace calwf

#endif // CALWF_IO_BAWEMETADATA_H