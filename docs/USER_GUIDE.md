# User Guide

> CALWF — How to install, run, and use the system.

---

## 1. Installation

### 1.1 Prerequisites

| Requirement | Version |
|---|---|
| OS | Windows 10/11 (64-bit) |
| CMake | 3.16+ |
| Compiler | g++ 13.1.0+ (MinGW-w64) |
| Or | MSVC 2022+ |

### 1.2 Clone

```powershell
git clone https://github.com/Valentina-Yue/CALWF.git
cd CALWF
```

### 1.3 Prepare Data

CALWF needs three data sources (see `DATA_FORMAT.md`):

- SWECCL (Chinese student corpus)
- BAWE (UK academic corpus)
- LLM outputs (180 samples)

Place them under `data/`, or configure custom paths in `config.json`.

### 1.4 Configure

Edit `config.json`:

```json
{
    "data_root": "D:/CALWF/data",
    "raw_root": "C:/your/path/to/corpora"
}
```

### 1.5 Build

```powershell
.\build.bat
```

Or manually:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

### 1.6 Run

```powershell
.\build\bin\calwf_cli.exe
```

---

## 2. Quick Start (Teacher Mode)

### 2.1 Detect a Student Essay

1. Launch `calwf_cli.exe`
2. Main menu → `3` (Text Analysis)
3. → `3` (Paste Text Analysis)
4. Paste the essay
5. Type `END` on its own line

System outputs:

```
╔══════════════════════════════════════════════════╗
║ Paste Text Analysis                               ║
╚══════════════════════════════════════════════════╝
Document ID    : PASTE_1791511771979810000
Source         : Unknown
─────────────────────────────────────
AI suspicion   : 57.79 %
Human-likeness : 42.21 %
SWECCL anchor  : 63.21 %
BAWE anchor    : 52.60 %
Verdict        : Uncertain
Risk level     : Low
─────────────────────────────────────
Model attribution (top-3):
  Qwen        dist 2.43  conf 41.01 %
  DeepSeek    dist 2.53  conf 39.46 %
  Doubao      dist 5.10  conf 19.53 %
─────────────────────────────────────
Risk words hit:
  - always
  - in conclusion
```

### 2.2 Interpreting Results

| Metric | Meaning |
|---|---|
| AI suspicion > 60% | Highly likely AI-generated. Manual review recommended. |
| Human-likeness > 65% | Likely human-written. |
| Between | Uncertain. Check the evidence chain. |

**Note**: CALWF is a decision-support tool, not a verdict. Final judgment should be made by a teacher.

---

## 3. Advanced Usage

### 3.1 Analyze a Pre-Existing LLM Document

```
Main menu → 3 → 1
   → Choose model
   → Choose prompt
   → Choose run
```

### 3.2 Analyze a File on Disk

```
Main menu → 3 → 2
   → Enter file path
```

### 3.3 Batch Analysis

```
Main menu → 3 → 4
   → Confirm
```

Analyzes all 180 LLM docs, prints per-model summary.

### 3.4 View Model Profiles

```
Main menu → 2 → 1   (overview)
Main menu → 2 → 2   (detail)
```

### 3.5 Export Reports

```
Main menu → 4 → 1
   → Choose scope
```

Generates:

- `report_Ndocs.md` (Markdown)
- `report_Ndocs.html` (HTML with CSS)
- `report_Ndocs.csv` (Excel-openable)

Open HTML with any browser:

```powershell
start "D:\CALWF\data\reports\report_5docs.html"
```

---

## 4. Understanding the Scores

### 4.1 AI Suspicion

**Formula**:

```
anchor_dev = 1 - max(sweccl_cosine, bawe_cosine)
suspicion = anchor_dev + vocab_penalty + hapax_penalty + longword_penalty
```

Higher = more likely AI.

### 4.2 Human-Likeness

```
human_likeness = 1 - ai_suspicion
```

Higher = more likely human.

### 4.3 SWECCL Anchor

Cosine similarity between the document's word frequency and the SWECCL corpus.

Higher = more like a Chinese student.

### 4.4 BAWE Anchor

Cosine similarity between the document's word frequency and the BAWE corpus.

Higher = more like a native academic writer.

### 4.5 Verdict

| Verdict | Rule |
|---|---|
| AI | suspicion > 0.60 |
| Human | suspicion < 0.35 |
| Uncertain | 0.35 ≤ suspicion ≤ 0.60 |

### 4.6 Risk Level

| Level | Trigger |
|---|---|
| High | Prompt residue detected OR extreme circular repetition (>30%) |
| Medium | Risk word ratio > 2% |
| Low | Otherwise |

---

## 5. FAQ

**Q1: `std::bad_alloc` on startup?**

A: Out of memory. Mitigations:

- Reduce `REF_LIMIT` in `Menu.cpp` (default 100)
- Or reduce the number of loaded corpora

**Q2: Cannot load SWECCL?**

A: Check `config.json`'s `raw_root`. It should point to the parent directory of SWECCL and BAWE, not SWECCL itself.

**Q3: First run is slow?**

A: First run builds n-gram caches (~60 s). Subsequent runs load from cache (~3 s).

Cache files:

- `data/index/sweccl.ngram`
- `data/index/bawe.ngram`

**Q4: Result is Uncertain. What to do?**

A: Check the evidence chain:

- If top-1 model confidence > 50%, lean AI
- If top-1 and top-2 are very close, could be a human imitating AI
- Look at risk words for prompt residue

**Q5: Can CALWF detect Chinese essays?**

A: No. CALWF is trained specifically for English essays. Other languages require redesigning features.

**Q6: Can CALWF detect outputs of GPT-4 / Claude?**

A: No, it's calibrated for 5 Chinese LLMs (DeepSeek, Doubao, ChatGLM, Kimi, Qwen). Adding new models requires collecting samples and rebuilding fingerprints.

**Q7: How accurate is CALWF?**

A: In our test set:

- AI vs. Human: 7/7 correct
- Model attribution: 5/5 correct

But CALWF is a tool, not a judge. Manual review is always recommended.

---

## 6. Capability Boundaries

### 6.1 Can Do

- Detect whether an English essay is AI-generated
- Identify the most likely source model
- Provide an explainable evidence chain
- Compare styles among students, natives, and AIs

### 6.2 Cannot Do

- Detect non-English essays
- 100% accuracy (it's a decision-support tool)
- Detect other LLMs (not in the fingerprint set)
- Online / real-time detection (fully offline)

---

## 7. Example Workflow

**Scenario**: A teacher receives a suspicious essay

**Step 1 — Paste the essay:**

```
Main menu → 3 → 3
[Paste essay]
END
```

**Step 2 — Read the result:**

```
AI suspicion   : 78.92 %
Human-likeness : 21.08 %
Verdict        : AI
```

**Step 3 — Check evidence:**

```
Model attribution: ChatGLM (54.57%)
Risk words: never
Similar docs: SWECCL_A104NGG, SWECCL_A101SD, SWECCL_A101HZKD
```

**Step 4 — Manual review:**

- Does the style match previous student work?
- Are there factual errors typical of LLMs?
- Does the student's other work look like this?

**Step 5 — Discussion with student.**

**Note**: CALWF provides evidence, not a verdict.

---

## 8. Report Export

### 8.1 Markdown

Structured with headers, tables, bullet lists. Viewable in any text editor or GitHub.

### 8.2 HTML

- Progress bars for AI suspicion
- Color-coded verdicts (red / green / yellow)
- Cards for each document
- Evidence chain visualization

Open in browser:

```powershell
start "D:\CALWF\data\reports\report_Ndocs.html"
```

### 8.3 CSV

17 columns, Excel / SPSS / R compatible.

Columns include:

- `doc_id`, `source`, `ai_suspicion`, `human_likeness`
- `sweccl_anchoring`, `bawe_anchoring`, `verdict`, `risk_level`
- top1/2/3 model + confidence
- `risk_word_hits`, `top_sweccl`, `top_bawe`

---

## 9. Performance

| Operation | Time |
|---|---|
| Startup (cached) | < 5 s |
| Startup (first) | ~60 s |
| Single document analysis | < 100 ms |
| Batch analysis (180 docs) | < 10 s |

---

## 10. Reporting Issues

Found a bug? Open an issue at:
`https://github.com/Valentina-Yue/CALWF/issues`

Please include:

- What you did
- What you expected
- What actually happened
- Your OS and compiler version

---

**Version**: v1.0
**Last updated**: 2026-10-08