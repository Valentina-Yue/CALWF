# Data Structures & Algorithms

> CALWF — a C++17 system built on classic CS fundamentals.

---

## Overview

CALWF is intentionally built **without third-party libraries** to showcase core computer science. This document lists every data structure and algorithm used, with rationale and complexity.

---

## 1. Data Structures

### 1.1 Classes & Encapsulation

**12 core classes**:

| Class | Header | Responsibility |
|---|---|---|
| `Token` | `core/Token.h` | Atomic text unit |
| `Sentence` | `core/Sentence.h` | Sequence of tokens |
| `Paragraph` | `core/Paragraph.h` | Sequence of sentences |
| `Document` | `core/Document.h` | Unified document model |
| `Corpus` | `core/Corpus.h` | Collection + hash index |
| `FeatureVector` | `features/FeatureVector.h` | 32-dim vector |
| `ModelProfile` | `model/ModelProfile.h` | Model fingerprint |
| `AnalysisResult` | `analysis/AnalysisResult.h` | Analysis output |
| `Analyzer` | `analysis/Analyzer.h` | Analysis engine |
| `CorpusAnchor` | `features/CorpusAnchor.h` | n-gram statistics |
| `Menu` | `cli/Menu.h` | Interactive CLI |
| `Logger` | `utils/Logger.h` | Singleton logger |

**Principle**: Private data + public methods + const-correctness.

### 1.2 Vector (Dynamic Array)

**Used everywhere**:

| Location | Element |
|---|---|
| `Sentence::tokens` | `Token` |
| `Paragraph::sentences` | `Sentence` |
| `Document::paragraphs` | `Paragraph` |
| `Corpus::docs` | `Document` |
| `FeatureVector::toVector()` | `double` |
| `ModelProfile::samples` | `FeatureVector` |
| `AnalysisResult::model_ranking` | `ModelScore` |

**Complexity**:

- Random access: O(1)
- Push back: amortized O(1)
- Insert/delete in middle: O(n)

**Why not list**: cache locality wins for our access patterns.

### 1.3 Hash Table (std::unordered_map / unordered_set)

**Used for**:

| Purpose | Location |
|---|---|
| n-gram frequency | `CorpusAnchor::unigram_/bigram_/trigram_` |
| Doc ID → index | `Corpus::id_index` |
| Word frequency | `LexicalFeatures`, `DiscourseFeatures` |
| Risk word set | `SafetyFeatures::g_risk_singles` |
| Modal verbs set | `SyntacticFeatures::modalVerbs()` |
| Connectives set | `DiscourseFeatures::connectivesSingle()` |

**Complexity**:

- Insert / lookup: amortized O(1)
- Worst case: O(n) (all collisions)

**Why not map (red-black tree)**: we don't need ordered traversal. For 5M n-grams, hash table O(1) vs tree O(log n) matters.

### 1.4 String

**Used for**: text storage, n-gram keys, file paths.

**Key operations**:

| Operation | Complexity |
|---|---|
| `substr` | O(n) |
| `find` | O(n*m) worst |
| `compare` | O(n) |

**Optimization**: `reserve()` + `ostringstream` for heavy concatenation.

### 1.5 State Machine

**Used by**:

- `CSVParser` (CSV parsing)
- `SentenceSplitter` (sentence boundary detection)

**CSV states**:

```
FIELD_START       — at start of field
IN_UNQUOTED       — inside unquoted field
IN_QUOTED         — inside quoted field
QUOTE_IN_QUOTED   — saw one quote inside quoted field
```

**Complexity**: O(n), single pass.

### 1.6 Sliding Window

**Used by**:

- Phrase matching (`matchPhrase`)
- n-gram extraction
- Circular repetition detection

**Example** (bigram extraction):

```cpp
for (size_t i = 0; i + 2 <= N; ++i) {
    std::string key = words[i] + " " + words[i + 1];
    ++bigram_[key];
}
```

**Complexity**: O(n * m), n=text length, m=phrase length.

### 1.7 Sorting (std::sort)

**Used for**:

- Top-K similar documents
- Model confidence ranking
- Feature importance ranking

**Algorithm**: Introsort (quicksort + heapsort + insertion sort).

**Complexity**: O(n log n).

### 1.8 Heap / Priority Queue (implicit)

**Used for**: Top-K queries.

Currently using partial sort (`std::sort` + take first K) because K is small (3-5).

For very large data, `std::priority_queue` gives O(n log K) Top-K.

### 1.9 Trie (implicit)

**Where**: `CorpusAnchor`'s n-gram tables are logically a one-level trie.

**Implementation**: `unordered_map<string, uint32_t>` where key is the full n-gram string.

**Why not explicit trie**: no prefix search needed. Hash table is simpler and faster.

### 1.10 Graph (planned)

**Purpose**: Model similarity network.

**Planned**:

- Adjacency list of model pairs
- BFS / DFS traversal
- Minimum spanning tree (model lineage)
- PageRank (central models)

**Current**: pairwise distances in `ModelProfile` form a complete weighted graph.

---

## 2. Algorithms

### 2.1 KMP String Matching

**File**: `utils/StringUtils.cpp`

```cpp
std::vector<std::size_t> kmpFindAll(const std::string& text,
                                    const std::string& pattern);
bool kmpContains(const std::string& text, const std::string& pattern);
```

**Idea**: Preprocess pattern to compute `next[]` (longest proper prefix that is also suffix). Use it to skip redundant comparisons.

**Complexity**:

- Time: O(n + m)
- Space: O(m)

**Used for**: template phrase and risk word lookup.

**Core code**:

```cpp
static std::vector<int> buildNext(const std::string& pattern) {
    std::vector<int> next(pattern.size(), 0);
    for (std::size_t i = 1, j = 0; i < pattern.size(); ++i) {
        while (j > 0 && pattern[i] != pattern[j]) j = next[j - 1];
        if (pattern[i] == pattern[j]) ++j;
        next[i] = static_cast<int>(j);
    }
    return next;
}
```

### 2.2 Levenshtein Distance (Edit Distance)

**File**: `utils/StringUtils.cpp`

```cpp
int editDistance(const std::string& a, const std::string& b);
```

**Idea**: Minimum single-char edits (insert/delete/substitute) to transform `a` into `b`.

**DP recurrence**:

```
dp[i][j] = min(
    dp[i-1][j] + 1,       // delete
    dp[i][j-1] + 1,       // insert
    dp[i-1][j-1] + cost   // substitute (cost = 0 or 1)
)
```

**Complexity**:

- Time: O(mn)
- Space: O(min(m,n)) (rolling array)

**Used for**: spelling check, string similarity.

### 2.3 Longest Common Subsequence (LCS)

**File**: `utils/StringUtils.cpp`

```cpp
int lcsLength(const std::string& a, const std::string& b);
```

**DP recurrence**:

```
if a[i-1] == b[j-1]:
    dp[i][j] = dp[i-1][j-1] + 1
else:
    dp[i][j] = max(dp[i-1][j], dp[i][j-1])
```

**Complexity**: O(mn) time, O(n) space (rolling array).

**Used for**: text similarity, plagiarism detection.

### 2.4 Weighted Euclidean Distance

**File**: `analysis/Analyzer.cpp`

**Formula**:

```
d(a, b) = sqrt( sum_j ((a_j - b_j) / sigma_j)^2 )
```

where `sigma_j` is the mean stddev of feature `j` across all models.

**Rationale**: Features have very different scales (word_count ~100, punctuation_density ~0.1). Weighting by stddev ensures each dimension contributes equally.

**Complexity**: O(d), d = 32.

### 2.5 kNN Classification

**File**: `analysis/Analyzer.cpp`

```cpp
std::vector<std::pair<std::string, double>>
knnAttribute(const FeatureVector& fv, std::size_t k) const;
```

**Algorithm**:

1. Compute distance to all samples (180)
2. Take K=5 nearest
3. Weighted voting by `1/(d+ε)`
4. Normalize to confidence

**Complexity**:

- Time: O(N * d), N = 180, d = 32
- Space: O(N)

**Why kNN**:

- No training needed
- Directly uses samples
- Interpretable ("it looks like 3 of these 5 samples")

**Result**: 5/5 models self-identified correctly.

### 2.6 Softmax Normalization

**File**: `analysis/Analyzer.cpp`

**Formula**:

```
confidence_i = exp(-(d_i - d_min) / T) / sum_j exp(-(d_j - d_min) / T)
```

**Temperature**: T = mean_distance_diff * 0.3.

**Why**: Converts distances to probability distribution. Temperature controls sharpness.

### 2.7 n-gram Coverage

**File**: `features/CorpusAnchor.cpp`

```cpp
double CorpusAnchor::ngramCoverage(const Document& doc, int n) const;
```

**Algorithm**:

1. Iterate all n-grams in document
2. Count how many are in corpus n-gram table
3. Coverage = hit / total

**Complexity**: O(n * m).

**Used for**: core anchoring metric.

### 2.8 Cosine Similarity

**File**: `features/CorpusAnchor.cpp`

**Formula**:

```
cos(a, b) = (a · b) / (|a| * |b|)
```

**Optimization**: only compute corpus norm over doc's words (dot product contributions outside are 0).

**Complexity**: O(unique_words).

### 2.9 Sentence Boundary Detection (State Machine)

**File**: `text/SentenceSplitter.cpp`

**Rules**:

- Strong punctuation (`.`, `!`, `?`) triggers boundary check
- Skip abbreviations (Mr., Dr., etc.)
- Boundary if followed by space + uppercase (or digit)
- Skip inside quotes / brackets

**Complexity**: O(n).

### 2.10 Paragraph Detection

**File**: `text/SentenceSplitter.cpp`

**Rules**:

- Blank line → paragraph ends
- Leading indentation → new paragraph
- Short first line (< 80 chars) + no indent → title (standalone paragraph)
- Otherwise → continue current paragraph

**Complexity**: O(n).

### 2.11 AI Suspicion Formula

**File**: `analysis/Analyzer.cpp`

```
anchor_dev = 1 - max(sweccl_cosine, bawe_cosine)

vocab_penalty (STTR):
    sttr > 0.70 → 0.15
    sttr > 0.60 → 0.08
    sttr > 0.55 → 0.03

hapax_penalty:
    hapax > 0.85 → 0.10
    hapax > 0.78 → 0.05

longword_penalty:
    longword > 0.38 → 0.08
    longword > 0.32 → 0.04

ai_suspicion = min(1, anchor_dev + vocab_penalty
                     + hapax_penalty + longword_penalty)
human_likeness = 1 - ai_suspicion
```

**Calibration data**:

| Source | STTR | Hapax | Long word |
|---|---|---|---|
| SWECCL | 0.54 | 0.70 | 0.23 |
| BAWE | 0.41 | 0.65 | 0.28 |
| LLM | 0.68-0.84 | 0.79-0.88 | 0.36-0.49 |

### 2.12 Risk Level Formula

**File**: `analysis/Analyzer.cpp`

```
if prompt_residue > 0:           risk = High
else if circular_rep > 0.30:     risk = High
else if risk_word_ratio > 0.02:  risk = Medium
else if risk_word_ratio > 0.01:  risk = Low (record reason)
else:                             risk = Low
```

**Rationale**: Only prompt residue or extreme circularity are "High". Absolute words (never, always) in academic writing are not high-risk.

---

## 3. Complexity Summary

| Operation | Complexity |
|---|---|
| Load SWECCL (3146 txt) | ~1 s |
| Load BAWE (2751 CSV) | ~30 s (first time) |
| Build n-gram cache | ~60 s (first time) |
| Load n-gram cache | ~3 s |
| Extract features (1 doc) | ~1 ms |
| Anchor (1 doc) | ~5 ms |
| Full analysis (1 doc) | ~20 ms |
| kNN (180 samples) | ~5 ms |
| Batch analysis (180 docs) | ~4 s |

---

## 4. Design Trade-offs

### 4.1 Hash Table vs Balanced Tree

**Choice**: Hash table (`unordered_map`).

**Why**: n-gram tables only need lookup, not ordered traversal.

**Cost**: Higher memory overhead (load factor), no range queries.

### 4.2 n-gram Table vs Full Trie

**Choice**: Flat n-gram table.

**Why**: Simpler, no prefix search needed. Hash table directly maps phrase → frequency.

**Cost**: No prefix search capability (not needed).

### 4.3 kNN vs Decision Tree

**Choice**: kNN.

**Why**: No training, direct use of samples, interpretable.

**Cost**: O(N) inference (fine for N=180).

### 4.4 Weighted vs Plain Euclidean Distance

**Choice**: Weighted by stddev.

**Why**: Features have different scales. Without weights, large-magnitude features dominate.

**Cost**: Need to compute per-feature stddev.

---

**Version**: v1.0
**Last updated**: 2026-10-08