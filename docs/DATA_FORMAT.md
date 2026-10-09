# Data Format

> CALWF — Data sources, formats, and preparation guide.

---

## Overview

CALWF consumes data from three sources:

| Source | Format | Size | Purpose |
|---|---|---|---|
| SWECCL | `.txt` (with tags) | 3,146 files | Chinese student baseline |
| BAWE | `.csv` (per-word) + metadata | 2,751 files | Native academic baseline |
| LLM | `.txt` (plain) | 180 files | Test samples |

**Note**: Raw corpora are **not** distributed with this repository due to copyright. This document describes their format so you can prepare them.

---

## 1. SWECCL (Written English Corpus of Chinese Learners)

### 1.1 Source

- **Provider**: Nanjing University
- **License**: Academic use only
- **URL**: Not publicly downloadable (academic license required)
- **Original book**: 文秋芳, 王立非, 梁茂成. 中国学生英语口笔语语料库. 外语教学与研究出版社, 2009.

### 1.2 Directory Structure

```
Raw Data (WECCL)/
├── Argumentation/          # 2,656 files
│   ├── A101FYSY.txt
│   ├── A101HZKD.txt
│   └── ...
├── Years 1-4 Essays/       # 489 files
│   ├── Argumentation/
│   │   ├── Year 1/
│   │   ├── Year 2/
│   │   ├── Year 3/
│   │   └── Year 4/
│   ├── Exposition/
│   │   ├── Year 1/
│   │   ├── Year 2/
│   │   └── Year 3/
│   └── Narration/
│       ├── Year 1/
│       ├── Year 2/
│       ├── Year 3/
│       └── Year 4/
└── tree.txt
```

### 1.3 File Format

Each `.txt` file begins with a **tag line** in angle brackets:

```
<WCOMP><ARG><GRADE1><YEAR03><TIMED><SCORE?><FYSY><LENGTH300W>
```

**Tag reference**:

| Tag | Meaning |
|---|---|
| `<WCOMP>` | Written Composition |
| `<ARG>` / `<EXP>` / `<NAR>` | Argumentation / Exposition / Narration |
| `<GRADE1>` ~ `<GRADE4>` | Year 1 ~ Year 4 |
| `<YEAR03>` | Entry year 2003 |
| `<TIMED>` / `<UNTIMED>` | Timed / Untimed |
| `<SCORE?>` | Score unknown |
| `<FYSY>` etc. | School code (FYSY, HZKD, JND, ND, NGD, NGG, SD, XSD, NGSD) |
| `<LENGTH300W>` | Target length in words |

**Body tags** (in-text annotations):

| Tag | Meaning |
|---|---|
| `<gr-xxx>` | Grammar error |
| `<sp-xxx>` | Spelling error |
| `<mis-xxx>` | Missing word |
| `<a>` | Article correction |

### 1.4 Quirks & Handling

**First line is not always the tag line.** CALWF's importer scans the first **10 lines** for the first line starting with `<` that contains `<WCOMP>`.

**Examples of variants**:

| File | First line |
|---|---|
| `A101FYSY.txt` | `<WCOMP><ARG>...` (direct) |
| `A101NGD.txt` | `A101NGD` (filename), tag on line 2 |
| `A122FYSY.txt` | empty line, tag on line 2 |
| `A202FYSY.txt` | `����<WCOMP><ARG>...` (garbled prefix) |

### 1.5 Import Statistics

- **Total scanned**: 3,146 files
- **Imported**: 3,139
- **Failed**: 6 (all either malformed or marked "incomplete")

---

## 2. BAWE (British Academic Written English)

### 2.1 Source

- **Provider**: University of Warwick / Reading / Oxford Brookes
- **License**: Academic use only
- **URL**: https://ota.ahds.ac.uk/headers/2539.xml
- **Documentation**: BAWE Corpus Manual v3 (2010)

### 2.2 Directory Structure

The **CSV categorized** version is recommended:

```
BAWE_csv_categorized/
├── AH/                 # Arts and Humanities
│   ├── level_1/
│   ├── level_2/
│   ├── level_3/
│   └── level_4/
├── LS/                 # Life Sciences
│   ├── level_1/
│   ├── ...
├── PS/                 # Physical Sciences
│   └── ...
└── SS/                 # Social Sciences
    └── ...
```

### 2.3 CSV Format

Each `.csv` file is a **per-word** table with this header:

```csv
"sentence_number","word_number","word","lemma","pos","ner","dep_on","dep"
```

**Example**:

```csv
1,1,"Racism","racism","NN","MISC",2,"nsubj"
1,2,"is","be","VBZ","O",0,"ROOT"
1,3,"still","still","RB","O",2,"advmod"
```

| Column | Meaning |
|---|---|
| `sentence_number` | Sentence index within document |
| `word_number` | Word index within sentence |
| `word` | Raw word |
| `lemma` | Lemma (base form) |
| `pos` | Penn Treebank POS tag |
| `ner` | Named entity tag |
| `dep_on` | Dependency parent (word_number) |
| `dep` | Dependency relation |

### 2.4 Metadata CSV

File: `bawe_metadata.csv`, 27 columns, 2,767 rows.

| Column | Example | Note |
|---|---|---|
| `level` | 1 | 1-4 |
| `disciplinary group` | LS | AH / LS / PS / SS |
| `genre family` | essay | 13 types |
| `discipline` | Agriculture | 35 disciplines |
| `L1` | Chinese Cantonese | Native language |
| `title` | Principal-agent Problem Essay | |
| `module` | Economics I | |
| `course` | Food Science with Business | |
| `education` | OSa | UKa / OSa / UKn |
| `student` | 6008 | |
| `id` | 6008h | Matches CSV filename |
| `date` | 2004/2/1 | |
| `grade` | D | M (merit) / D (distinction) |
| `words` | 1549 | |
| `s-units` | 48 | |
| `p-units` | 12 | |
| `gender` | f | m / f |
| `year of birth` | 1985 | |

**Genre families** (13 types):

```
case study, critique, design specification, empathy writing, essay, exercise, explanation, literature review, methodology recount, narrative recount, problem question, proposal, research report
```

### 2.5 Import Statistics

- Total CSV files: 2,751
- Imported: 2,751
- Metadata entries: 2,761
- Metadata without CSV: 10

---

## 3. LLM Outputs

### 3.1 Directory Structure

```
data/llm/
├── DeepSeek/
│   ├── p001_run1.txt
│   ├── p001_run2.txt
│   ├── p001_run3.txt
│   └── ...
├── Doubao/
├── ChatGLM/
├── Kimi/
└── Qwen/
```

### 3.2 File Naming

```
<model>/<prompt_id>_run<N>.txt
```

Example: `DeepSeek/p001_run1.txt`

### 3.3 Prompt Matrix

3 topics × 4 levels × 5 models × 3 runs = 180 samples.

| Prompt ID | Genre | Topic | Target Level |
|---|---|---|---|
| p001 | Argumentation | Degree certificates | CET4 |
| p002 | Argumentation | Degree certificates | CET6 |
| p003 | Argumentation | Degree certificates | TEM4 |
| p004 | Argumentation | Degree certificates | TEM8 |
| p005 | Exposition | AI and language learning | CET4 |
| p006 | Exposition | AI and language learning | CET6 |
| p007 | Exposition | AI and language learning | TEM4 |
| p008 | Exposition | AI and language learning | TEM8 |
| p009 | Narration | Unforgettable English learning | CET4 |
| p010 | Narration | Unforgettable English learning | CET6 |
| p011 | Narration | Unforgettable English learning | TEM4 |
| p012 | Narration | Unforgettable English learning | TEM8 |

### 3.4 Metadata CSV

File: `llm_meta.csv`, 13 columns, 180 rows.

| Column | Example |
|---|---|
| `doc_id` | LLM_DeepSeek_p001_r1 |
| `model_name` | DeepSeek |
| `model_version` | v3 |
| `prompt_id` | p001 |
| `topic` | Degree certificates |
| `genre` | Argumentation |
| `target_level` | CET4 |
| `run_number` | 1 |
| `deep_thinking` | false |
| `web_access` | false |
| `date_generated` | 2026-10-07 |
| `file_path` | data/llm/DeepSeek/p001_run1.txt |
| `notes` | |

### 3.5 Collection Protocol

To ensure reproducibility:

1. Open model web UI, start a new session
2. Disable web access
3. Disable deep thinking (or record as true)
4. Copy prompt verbatim
5. Wait for full generation
6. Copy entire response
7. Save to `<model>/<prompt_id>_run<N>.txt`
8. Start a new session before next run
9. Fill metadata row

---

## 4. Configuration Files

### 4.1 config.json

```json
{
    "data_root": "D:/CALWF/data",
    "raw_root": "C:/Users/asus/Desktop/Linguistics/Corpus/Corpus sample"
}
```

- `data_root` — Project data directory
- `raw_root` — Parent directory of SWECCL and BAWE

### 4.2 prompts.json

Contains all 12 prompts with full text.

### 4.3 risk_words.txt

One word or phrase per line. Lines starting with `#` are comments.

**Categories**:

- Absolute words (always, never, everyone)
- Empty clichés (in conclusion, as we all know)
- Prompt residue (as an ai, i'm sorry)
- Academic misconduct hints (plagiarism)

---

## 5. n-gram Cache Format

Location: `data/index/sweccl.ngram`, `data/index/bawe.ngram`

Binary format:

```
Offset  Size    Content
0       8       Magic "CALWFNG1"
8       8       uint64  total_tokens
16      8       uint64  unigram_count
24      8       uint64  bigram_count
32      8       uint64  trigram_count
40      ...     Unigram table
...     ...     Bigram table
...     ...     Trigram table

Each entry:
    uint32  key_length
    bytes   key (UTF-8)
    uint32  frequency
```

- Build time: ~60 s first time.
- Load time: ~3 s.

---

## 6. Preparing Data

To run CALWF on your own machine:

1. Obtain SWECCL (requires academic license)
2. Obtain BAWE (freely available from Oxford Text Archive)
3. Collect LLM outputs (see §3.5)
4. Set `raw_root` in `config.json` to the parent directory of SWECCL and BAWE
5. Run the CLI — n-gram caches will be built on first use

---

## 7. Copyright Notes

**Do not redistribute**:

- SWECCL raw files
- BAWE raw files
- LLM full outputs (bulk)

**Allowed**:

- Code
- Documentation
- Configurations
- Small samples (e.g., 1-2 files per source)

---

**Version**: v1.0
**Last updated**: 2026-10-08