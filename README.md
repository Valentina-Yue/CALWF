<div align="center">

# CALWF · 语料锚定镜

**Corpus-Anchored LLM Writing Forensics**

*Expose LLM writing fingerprints with real corpora.*

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)]()

</div>

---

## What is CALWF?

CALWF is an **offline C++17 forensic analysis system** that detects whether an English essay is written by a human or by an LLM.

Instead of training a black-box model, CALWF uses **two real corpora as anchors**:

- **SWECCL** — 3,139 essays from Chinese English-major students
- **BAWE** — 2,751 academic essays from UK university students

Against these anchors, it audits the outputs of **5 major Chinese LLMs** (DeepSeek, Doubao, ChatGLM, Kimi, Qwen) across **32 linguistic features**.

### Key Results

| Task | Performance |
|---|---|
| AI vs. Human detection | **7 / 7** correct |
| Model attribution (5 models) | **5 / 5** correct |
| Single-document analysis | < 100 ms |
| Batch analysis (180 docs) | < 10 s |

---

## The Core Idea

> **Don't feed the corpus to the LLM. Use the corpus to audit the LLM.**

Traditional approach:

```
Corpus → Train model → Generate text
```

CALWF approach:

```
Corpus → Build n-gram statistics → Audit LLM output
```

This reverses the role of a corpus: from **training data** to **measurement baseline**.

---

## Key Finding

> **AI writing is neither like a typical Chinese student nor like a native academic writer.**

| Comparison | SWECCL coverage | BAWE coverage |
|---|---|---|
| SWECCL vs. itself | **1.0000** | 0.7918 |
| BAWE vs. itself | 0.4975 | **1.0000** |
| **LLM average** | **0.5247** | **0.6244** |

LLMs exhibit "over-lexicalization":

| Feature | SWECCL | BAWE | LLM |
|---|---|---|---|
| Long word ratio (≥7 chars) | 0.23 | 0.28 | **0.40** |
| Hapax ratio | 0.70 | 0.65 | **0.84** |
| STTR | 0.54 | 0.41 | **0.74** |
| -ing structures | 0.023 | 0.027 | **0.040** |

---

## Features

- **Dual-corpus anchoring** — SWECCL + BAWE as dual coordinate systems
- **32-dimensional feature space** — surface / lexical / syntactic / discourse / anchoring / safety
- **Model fingerprinting** — 5 LLMs, 36 samples each, kNN attribution
- **Explainable evidence chain** — template phrases, risk words, similar documents, anomalous sentences
- **Multi-format reports** — Markdown / HTML / CSV
- **Fully offline** — no API calls, no network dependency

---

## Quick Start

### Prerequisites

- Windows 10/11 (64-bit)
- CMake 3.16+
- g++ 13.1.0+ (MinGW-w64)
- Or MSVC 2022+

### Build

```powershell
git clone https://github.com/Valentina-Yue/CALWF.git
cd CALWF
.\build.bat
```

### Run

```powershell
.\build\bin\calwf_cli.exe
```

### Interactive Menu

```
╔════════════════════════════════════════╗
║  CALWF · Corpus-Anchored Forensics      ║
╚════════════════════════════════════════╝

  1. Data Management
  2. Model Profiles
  3. Text Analysis
  4. Report Export

  0. Exit
```

### Paste-and-Detect (Teacher Mode)

```
Main menu → 3 → 3
Paste essay → Type END
```

The system outputs:

- AI suspicion score
- Human-likeness index
- SWECCL / BAWE anchoring scores
- Top-3 model attribution
- Risk level + evidence chain

### Example Output

```
╔══════════════════════════════════════════════════╗
║ DeepSeek / p001 / run1                            ║
╚══════════════════════════════════════════════════╝
Document ID    : LLM_DeepSeek_p001_r1
Source         : LLM
─────────────────────────────────────
AI suspicion   : 74.51 %
Human-likeness : 25.49 %
SWECCL anchor  : 53.49 %
BAWE anchor    : 41.64 %
Verdict        : AI
Risk level     : Low
─────────────────────────────────────
Model attribution (top-3):
  DeepSeek    dist 1.64  conf 60.80 %
  Doubao      dist 3.69  conf 26.97 %
  ChatGLM     dist 8.11  conf 12.22 %
─────────────────────────────────────
Risk words hit:
  - never
─────────────────────────────────────
Most similar SWECCL docs:
  - SWECCL_A104NGG
  - SWECCL_A101SD
  - SWECCL_A101HZKD
```

---

## Architecture

```
┌──────────────────────────────────────────────────┐
│              CLI Interface                        │
│  Menu / ReportGenerator                          │
├──────────────────────────────────────────────────┤
│              Analysis Layer                       │
│  Analyzer / ModelProfile                         │
├──────────────────────────────────────────────────┤
│              Feature Layer                        │
│  FeatureVector / 6 feature extractors            │
├──────────────────────────────────────────────────┤
│              Text Layer                           │
│  SentenceSplitter / CorpusAnchor                 │
├──────────────────────────────────────────────────┤
│              Data Layer                           │
│  Document / Corpus / Token                       │
├──────────────────────────────────────────────────┤
│              Import Layer                         │
│  SwecclImporter / BaweImporter / LlmImporter     │
├──────────────────────────────────────────────────┤
│              Utility Layer                        │
│  StringUtils / FileUtils / Logger                │
└──────────────────────────────────────────────────┘
```

See `ARCHITECTURE.md` for details.

---

## Project Structure

```
CALWF/
├── CMakeLists.txt          # Build script
├── build.bat               # One-click build
├── config.json             # Path configuration
├── docs/                   # Documentation
├── data/                   # Data (not in git)
│   ├── config/             # Prompts + risk words
│   ├── metadata/           # CSV metadata
│   ├── index/              # n-gram caches
│   ├── llm/                # 180 LLM outputs
│   └── reports/            # Generated reports
├── include/calwf/          # Headers
│   ├── core/               # Data models
│   ├── io/                 # Corpus importers
│   ├── text/               # Sentence splitter
│   ├── features/           # 32 features
│   ├── model/              # Model fingerprints
│   ├── analysis/           # Analysis engine
│   ├── report/             # Report generator
│   ├── cli/                # Interactive menu
│   └── utils/              # Utilities
└── src/                    # Sources
```

---

## Documentation

| Document | Description |
|---|---|
| `ARCHITECTURE.md` | System architecture and design |
| `ALGORITHMS.md` | Data structures and algorithms |
| `DATA_FORMAT.md` | Data format specification |
| `USER_GUIDE.md` | User guide and FAQ |

---

## Technical Stack

| Component | Technology |
|---|---|
| Language | C++17 |
| Build | CMake 3.16+ |
| Compiler | g++ 13.1.0 (MinGW-w64) |
| Standard library only | ✅ No third-party deps |
| GUI (planned) | Qt 6.10.2 |

---

## Feature Space (32 dimensions)

| Category | Count | Examples |
|---|---|---|
| Surface | 10 | Sentence length, punctuation density |
| Lexical | 5 | TTR, STTR, long word ratio, hapax ratio |
| Syntactic | 5 | Passive, clause markers, modals |
| Discourse | 4 | Connectives, template phrases, pronouns |
| Anchoring | 5 | n-gram coverage, cosine similarity |
| Safety | 3 | Risk words, prompt residue, repetition |

---

## Algorithms

- **Weighted Euclidean distance** — for feature vector comparison
- **kNN attribution** — k=5, weighted voting
- **Softmax normalization** — for confidence
- **n-gram coverage** — for corpus anchoring
- **Cosine similarity** — for word frequency distribution
- **KMP string matching** — for template phrases
- **Levenshtein distance / LCS** — for string similarity

See `ALGORITHMS.md` for details.

---

## Data Sources

| Corpus | Size | License |
|---|---|---|
| SWECCL | 3,139 essays | Academic use only |
| BAWE | 2,751 essays | Academic use only |
| LLM outputs | 180 samples | Generated offline |

**Note**: Raw corpora are not included in this repository due to copyright. See `DATA_FORMAT.md` for how to prepare data.

---

## Roadmap

- [x] Corpus importers (SWECCL / BAWE / LLM)
- [x] 32-feature extraction
- [x] Dual-corpus anchoring
- [x] Model fingerprinting (5 models)
- [x] kNN attribution
- [x] CLI interactive menu
- [x] Paste-and-detect mode
- [x] Multi-format reports (MD / HTML / CSV)
- [ ] Qt GUI
- [ ] Graph analysis (model similarity network)
- [ ] Decision tree classifier
- [ ] Support for more corpora

---

## License

MIT License — see `LICENSE`.

---

## Acknowledgement

- SWECCL corpus — Nanjing University
- BAWE corpus — University of Warwick / Reading / Oxford Brookes
- Built with Qt 6.10.2 + MinGW 13.1.0

---

<div align="center">

**CALWF — Shining real corpora on LLM writing.**

</div>