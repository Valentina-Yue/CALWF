// ============================================================
// CALWF - AnchoringFeatures 锚定特征
// ============================================================
//
// 文件名：include/calwf/features/AnchoringFeatures.h
// 作用：用 SWECCL 和 BAWE 两个语料库锚定器，
//       计算文档的双重锚定特征。
//
// 对应特征（FeatureVector 的 E 类）：
//   sweccl_ngram_coverage   文档 bigram 在 SWECCL 中的覆盖率
//   bawe_ngram_coverage     文档 bigram 在 BAWE 中的覆盖率
//   oov_rate                不在 SWECCL 词表中的词比例
//   sweccl_cosine           与 SWECCL 词频分布的余弦相似度
//   bawe_cosine             与 BAWE 词频分布的余弦相似度
//
// 这是"语料锚定镜"的核心：给任何一段文本，判断它更接近
// 中国学生语料（SWECCL）还是母语者学术语料（BAWE）。
// ============================================================

#ifndef CALWF_FEATURES_ANCHORINGFEATURES_H
#define CALWF_FEATURES_ANCHORINGFEATURES_H

#include <string>

#include "calwf/core/Document.h"
#include "calwf/features/CorpusAnchor.h"
#include "calwf/features/FeatureVector.h"

namespace calwf {
namespace features {

class AnchoringFeatures {
public:
    // 初始化（从磁盘加载或构建）
    // 如果缓存文件存在就加载，否则用 corpus 构建并保存
    static void initialize(
        const std::string& sweccl_corpus_dir,
        const std::string& bawe_meta_path,
        const std::string& bawe_csv_dir,
        const std::string& sweccl_cache,
        const std::string& bawe_cache
    );

    // 提取特征
    static void extract(const Document& doc, FeatureVector& fv);

    // 是否已初始化
    static bool isInitialized();

    // 调试：打印 SWECCL / BAWE 的规模
    static void printStats();

    // 暴露内部 anchor 引用（避免重复加载）
    static const CorpusAnchor& swecclAnchor();
    static const CorpusAnchor& baweAnchor();
};

} // namespace features
} // namespace calwf

#endif // CALWF_FEATURES_ANCHORINGFEATURES_H