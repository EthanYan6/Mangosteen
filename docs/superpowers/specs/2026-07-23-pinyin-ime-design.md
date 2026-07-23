# Pinyin IME Design (ChName + Compose) — Approach B

**Date:** 2026-07-23  
**Status:** approved for implementation

## Goal

Compact GB2312 T9 pinyin on:

1. Menu **ChName** (命名信道) — 10-byte name  
2. Messenger **COMPOSE** — 36-byte payload  

No other editors. No fullscreen IME overlay.

## Data

SPI OEM tables (MCU flash free):

- Index `0x100000` — 485 × 16 B T9 digit records (same as BG1JST reference)
- Hanzi blob `0x101F00` — packed GB2312 pairs
- Fonts `0x0A0000` / `0x0E0000` (existing)

## UX

| Context | Mode key | Cycle | Badge |
|---------|----------|-------|-------|
| ChName | `#` (`KEY_F` short) | abc → ABC → 123 → pinyin | `abc` / `ABC` / `123` / `pinyin` |
| Compose | `*` | B → b → 2 → pinyin | `B` / `b` / `2` / `pinyin` |

Backspace: Compose = `F`; ChName (pinyin) = `*`.  
Candidates: 1–2 small rows under the edit field only while composing/picking.  
T9: `2–9` digits, `↑↓` syllable/page, `1` enter pick, `1–9` select hanzi, `0` space.

## Module

`App/app/pinyin_ime.c` — CN engine only (no EN multitap duplicate).  
Hosts keep ASCII multitap; IME active only in `pinyin` mode.

## Out of scope

Callsign / Yan ID / DTMF. Phrase prediction. Fullscreen IME.
