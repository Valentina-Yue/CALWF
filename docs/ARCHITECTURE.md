# Architecture

> CALWF — Corpus-Anchored LLM Writing Forensics

---

## 1. Design Philosophy

CALWF is built around three principles:

1. **Offline-first** — no network, no API, no external service
2. **Explainable** — every verdict comes with evidence
3. **Composable** — each module can be used independently

---

## 2. Layered Architecture

```
┌──────────────────────────────────────────────────┐
│ CLI Interface                                     │
│ Menu / ReportGenerator                            │
├──────────────────────────────────────────────────┤
│ Analysis Layer                                    │
│ Analyzer / ModelProfile                           │
├──────────────────────────────────────────────────┤
│ Feature Layer                                     │
│ FeatureVector / 6 feature extractors              │
├──────────────────────────────────────────────────┤
│ Text Layer                                        │
│ SentenceSplitter / CorpusAnchor                   │
├──────────────────────────────────────────────────┤
│ Data Layer                                        │
│ Document / Corpus / Token                         │
├──────────────────────────────────────────────────┤
│ Import Layer                                      │
│ SwecclImporter / BaweImporter / LlmImporter       │
├──────────────────────────────────────────────────┤
│ Utility Layer                                     │
│ StringUtils / FileUtils / Logger                  │
└──────────────────────────────────────────────────┘
```

---

## 3. Data Flow

```
Raw Data
│
├─ SWECCL txt ─┐
├─ BAWE CSV  ──┼──→ Importers ──→ vector<Document>
└─ LLM txt   ──┘
│
↓
Preprocessing
│
↓
Feature Extraction
│
↓
FeatureVector (32-dim)
│
┌───────────────┼───────────────┐
↓               ↓               ↓
Model Profiles  Anchors         Safety
│               │               │
└───────────────┼───────────────┘
↓
Analyzer
│
↓
AnalysisResult
│
┌───────────────┼───────────────┐
↓               ↓               ↓
Markdown        HTML            CSV
```

---

## 4. Module Responsibilities

### 4.1 core

**Purpose**: Define the core data model.

| File | Responsibility |
|---|---|
| `Token.h` | Atomic unit (word/punctuation) |
| `Sentence.h` | Sequence of tokens |
| `Paragraph.h` | Sequence of sentences |
| `Document.h` | Unified document (SWECCL / BAWE / LLM) |
| `Corpus.h` | Collection of documents + hash index |

**Design principles**:

- Immutable where possible
- No side effects
- Lightweight (POD for `Token`)

### 4.2 io

**Purpose**: Import three data sources.

| File | Responsibility |
|---|---|
| `CSVParser` | CSV parsing (quotes, BOM, escapes) |
| `SwecclMeta` | Parse `<WCOMP><ARG>...` tags |
| `SwecclImporter` | Import SWECCL `.txt` |
| `BaweMetadata` | Load BAWE metadata CSV |
| `BaweImporter` | Import BAWE per-word CSV |
| `LlmImporter` | Import LLM outputs |

**Design principles**:

- Each importer independent
- Fault-tolerant: skip bad files, log warnings
- Unified output: `vector<Document>`

### 4.3 text

**Purpose**: Text preprocessing.

| File | Responsibility |
|---|---|
| `SentenceSplitter` | Split into sentences / paragraphs |

**Core algorithms**:

- Sentence boundary detection with state machine
- Paragraph detection by blank lines and indentation

### 4.4 features

**Purpose**: Extract 32 linguistic features.

| File | Responsibility |
|---|---|
| `FeatureVector` | Container for 32 features |
| `SurfaceFeatures` | 10 surface features |
| `LexicalFeatures` | 5 lexical features |
| `SyntacticFeatures` | 5 syntactic features |
| `DiscourseFeatures` | 4 discourse features |
| `CorpusAnchor` | n-gram statistics for anchoring |
| `AnchoringFeatures` | 5 anchoring features |
| `SafetyFeatures` | 3 safety features |

**Design principles**:

- Each extractor independent
- Uniform interface: `static void extract(const Document&, FeatureVector&)`
- Pure functions, no side effects

### 4.5 model

**Purpose**: Model fingerprints.

| File | Responsibility |
|---|---|
| `ModelProfile` | Mean/std/samples for each model |

**Core logic**:

- Group by `model_name`
- Compute mean and stddev per feature
- Keep all samples for kNN

### 4.6 analysis

**Purpose**: Analysis engine.

| File | Responsibility |
|---|---|
| `AnalysisResult` | Result structure |
| `Analyzer` | Core analysis pipeline |

**Core algorithms**:

- Weighted Euclidean distance
- kNN attribution
- AI suspicion formula
- Risk level formula

### 4.7 report

**Purpose**: Report generation.

| File | Responsibility |
|---|---|
| `ReportGenerator` | Markdown / HTML / CSV |

**Design principles**:

- `render` returns string
- `save` writes to file
- HTML has inline CSS (no external files)

### 4.8 cli

**Purpose**: Interactive menu.

| File | Responsibility |
|---|---|
| `Menu` | Main menu + submenus |

**Design principles**:

- Lazy loading (load only when needed)
- Fault-tolerant (never crash on bad input)
- Hierarchical navigation (always can return)

### 4.9 utils

**Purpose**: General utilities.

| File | Responsibility |
|---|---|
| `StringUtils` | KMP, edit distance, LCS |
| `FileUtils` | File I/O, directory traversal |
| `Logger` | Singleton logger |

---

## 5. Key Data Structures

### 5.1 Document

```cpp
class Document {
    std::string doc_id;
    SourceType  source;
    // ... metadata
    std::vector<Paragraph> paragraphs;
};
```

**Rationale**:

- Unified structure for SWECCL / BAWE / LLM
- vector for O(1) random access
- Optional fields (model_name only for LLM)

### 5.2 FeatureVector

```cpp
struct FeatureVector {
    // 32 doubles
    std::vector<double> toVector() const;
    static FeatureVector fromVector(const std::vector<double>&);
};
```

**Rationale**:

- Struct (POD-like) for performance
- `toVector()` for kNN / distance
- `featureNames()` for explainability

### 5.3 ModelProfile

```cpp
struct ModelProfile {
    std::string model_name;
    int         sample_count;
    FeatureVector mean;
    FeatureVector stddev;
    std::vector<FeatureVector> samples;  // for kNN
};
```

**Rationale**:

- Mean for fast nearest-centroid
- Samples for accurate kNN
- Memory: 5 models × 36 samples × 32 doubles ≈ 46 KB

### 5.4 AnalysisResult

```cpp
struct AnalysisResult {
    double ai_suspicion;
    double human_likeness;
    double sweccl_anchoring;
    double bawe_anchoring;
    std::string verdict;
    std::vector<ModelScore> model_ranking;
    std::string risk_level;
    // ... evidence chain
};
```

**Rationale**:

- All fields for a single analysis
- Evidence chain for explainability
- Serializable to JSON (planned)

---

## 6. Core Algorithms

### 6.1 Weighted Euclidean Distance

```cpp
double weightedDistance(const FeatureVector& a, const FeatureVector& b) {
    auto va = a.toVector(), vb = b.toVector();
    double sum = 0;
    for (size_t j = 0; j < va.size(); ++j) {
        double d = (va[j] - vb[j]) / sigma[j];  // sigma = stddev mean
        sum += d * d;
    }
    return sqrt(sum);
}
```

**Rationale**: Features have different scales (word count ~100, punctuation density ~0.1). Weighting by stddev normalizes contributions.

### 6.2 kNN Attribution

```cpp
std::vector<std::pair<std::string, double>>
knnAttribute(const FeatureVector& fv, size_t k) {
    // 1. Compute distances to all samples
    // 2. Take k nearest
    // 3. Weighted voting by 1/(d+ε)
    // 4. Normalize
}
```

**Complexity**: O(N × d), N=180, d=32.

**Why kNN**: No training, interpretable, works with small samples.

### 6.3 AI Suspicion Formula

```
anchor_dev = 1 - max(sweccl_cosine, bawe_cosine)
suspicion = anchor_dev + vocab_penalty + hapax_penalty + longword_penalty
suspicion = min(1, max(0, suspicion))
```

**Calibration basis**:

| Source | STTR | Hapax | Long word ratio |
|---|---|---|---|
| SWECCL | 0.54 | 0.70 | 0.23 |
| BAWE | 0.41 | 0.65 | 0.28 |
| LLM | 0.68-0.84 | 0.79-0.88 | 0.36-0.49 |

---

## 7. Performance

### 7.1 n-gram Caching

- First-time build: ~60 s. Subsequent load: ~3 s.

**Format**:

```
magic   8 bytes  "CALWFNG1"
uint64  total_tokens
uint64  unigram_count
uint64  bigram_count
uint64  trigram_count
--- per table ---
uint32 len
bytes  key
uint32 freq
```

### 7.2 Lazy Loading

CLI does not preload data:

- View profiles only → load LLM only
- Analyze → load everything

### 7.3 Reference Corpus Truncation

- Only use first 100 SWECCL + 100 BAWE docs as reference.
- Avoids OOM, maintains representativeness.

---

## 8. Error Handling

### 8.1 Import Layer

- File not found → log warning, skip
- Malformed format → log warning, skip
- Never break the whole import

### 8.2 Analysis Layer

- Empty document → return empty FeatureVector
- Missing anchors → skip anchoring features
- Invalid input → return Uncertain

### 8.3 CLI Layer

- Invalid menu choice → prompt, return
- Empty input → return to parent
- File not found → show path, return

---

## 9. Extensibility

### 9.1 Add a New Corpus

1. Add importer in `io/`
2. Output `vector<Document>`
3. Register in `CorpusAnchor`

### 9.2 Add a New Feature

1. Add extractor in `features/`
2. Add field to `FeatureVector`
3. Call from `Analyzer::extractAll`

### 9.3 Add a New Report Format

1. Add generator in `report/`
2. Call from `Menu::exportReport`

---

## 10. Build System

### 10.1 CMake Structure

```
CMakeLists.txt              # top-level
├── src/CMakeLists.txt      # library + exe
└── gui/CMakeLists.txt      # Qt GUI (optional)
```

### 10.2 Key Configurations

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static linking (Windows)
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

# Strict warnings
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
)
```

---

**Version**: v1.0
**Last updated**: 2026-10-08