// ============================================================
// CALWF - BAWE 元数据实现
// ============================================================

#include "calwf/io/BaweMetadata.h"
#include "calwf/io/CSVParser.h"
#include "calwf/utils/StringUtils.h"
#include "calwf/utils/Logger.h"
#include <algorithm>

namespace calwf {
namespace io {

// ------------------------------------------------------------
// 从 CSV 一行 + 表头，填充 BaweMetadataEntry
// ------------------------------------------------------------
static BaweMetadataEntry rowToEntry(
    const std::vector<std::string>& header,
    const std::vector<std::string>& row) {

    BaweMetadataEntry e;

    // 遍历列，按列名匹配
    for (std::size_t i = 0; i < header.size() && i < row.size(); ++i) {
        const std::string& key = header[i];
        const std::string& val = row[i];

        if (key == "level") {
            e.level = utils::toInt(val, -1);
        } else if (key == "disciplinary group") {
            e.disciplinary_group = val;
        } else if (key == "genre family") {
            e.genre_family = val;
        } else if (key == "discipline") {
            e.discipline = val;
        } else if (key == "L1") {
            e.l1 = val;
        } else if (key == "title") {
            e.title = val;
        } else if (key == "module") {
            e.module = val;
        } else if (key == "course") {
            e.course = val;
        } else if (key == "education (UKA: all secondary education in UK; Ukn: n years of secondary education in UK; OSA: all secondary education overseas)") {
            e.education = val;
        } else if (key == "education") {
            e.education = val;
        } else if (key == "student") {
            e.student = val;
        } else if (key == "id") {
            e.id = val;
        } else if (key == "date") {
            e.date = val;
        } else if (key == "grade") {
            e.grade = val;
        } else if (key == "words") {
            e.words = utils::toInt(val, 0);
        } else if (key == "s-units") {
            e.s_units = utils::toInt(val, 0);
        } else if (key == "p-units") {
            e.p_units = utils::toInt(val, 0);
        } else if (key == "gender") {
            e.gender = val;
        } else if (key == "year of birth") {
            e.year_of_birth = utils::toInt(val, -1);
        }
    }

    return e;
}

// ------------------------------------------------------------
// 加载元数据表
// ------------------------------------------------------------
bool BaweMetadataTable::loadFromFile(const std::string& path) {
    std::vector<std::string> header;
    auto rows = CSVParser::parseFileWithHeader(path, header);

    if (header.empty()) {
        CALWF_LOG_ERROR("BAWE 元数据加载失败：无法读取表头，路径 = %s",
                        path.c_str());
        return false;
    }

    // 去掉列名可能带的 BOM（CSVParser 已处理，但保险起见）
    if (!header.empty() && !header[0].empty()) {
        std::string& h0 = header[0];
        if (h0.size() >= 3 &&
            static_cast<unsigned char>(h0[0]) == 0xEF &&
            static_cast<unsigned char>(h0[1]) == 0xBB &&
            static_cast<unsigned char>(h0[2]) == 0xBF) {
            h0 = h0.substr(3);
        }
    }

    CALWF_LOG_INFO("BAWE 元数据：表头 %zu 列，数据 %zu 行",
                   header.size(), rows.size());

    int loaded  = 0;
    int skipped = 0;

    for (const auto& row : rows) {
        if (row.empty()) continue;

        BaweMetadataEntry e = rowToEntry(header, row);

        if (e.id.empty()) {
            ++skipped;
            continue;
        }

        table_[e.id] = std::move(e);
        ++loaded;
    }

    CALWF_LOG_INFO("BAWE 元数据加载完成：成功 %d 条，跳过 %d 条",
                   loaded, skipped);
    return true;
}

// ------------------------------------------------------------
// 查找
// ------------------------------------------------------------
const BaweMetadataEntry*
BaweMetadataTable::find(const std::string& id) const {
    auto it = table_.find(id);
    if (it == table_.end()) {
        return nullptr;
    }
    return &it->second;
}

// ------------------------------------------------------------
// 全部 ID
// ------------------------------------------------------------
std::vector<std::string> BaweMetadataTable::allIds() const {
    std::vector<std::string> ids;
    ids.reserve(table_.size());
    for (const auto& kv : table_) {
        ids.push_back(kv.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace io
} // namespace calwf