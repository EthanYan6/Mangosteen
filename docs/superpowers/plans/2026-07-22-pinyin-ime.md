# Pinyin IME Implementation Plan

> Inline execution in session (user requested 开发吧).

**Goal:** GB2312 pinyin input on ChName + Messenger COMPOSE using SPI tables at `0x100000`.

**Architecture:** `app/pinyin_ime.c` reads index/list; ChName (`edit_ime_mode`) and Messenger T9 (`mode==3`) share it. Display uses `0xA0000`/`0xE0000` fonts. Table image: `docs/font/uvk1_pinyin.bin`.

---

### Done

- [x] Spec `docs/superpowers/specs/2026-07-22-pinyin-ime-design.md`
- [x] `pack_uvk1_pinyin.py` + `uvk1_pinyin.bin`
- [x] `pinyin_ime.c/h` + CMake
- [x] Messenger T9 mode 3 + compose UI
- [x] ChName `*` mode cycle + candidates
- [x] Web「刷入拼音表」button

### Device test

1. Flash firmware, then flash `uvk1_pinyin.bin` via docs「刷入拼音表」
2. ChName: `F` → PY → type `ni` → UP/DOWN → MENU 选字
3. 信息→编辑信息: `*` → P → same
