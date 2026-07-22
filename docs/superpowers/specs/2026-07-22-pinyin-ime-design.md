# Pinyin IME Design (ChName + Compose)

**Date:** 2026-07-22

## Goal

Add GB2312 pinyin input on:

1. Menu **ChName** (命名信道) — 10-byte name buffer  
2. Messenger **COMPOSE** (编辑信息) — 36-byte payload buffer  

Display uses Mangosteen fonts at `0x0A0000` / `0x0E0000`.  
Lookup uses SPI tables at OEM addresses (`0x100000` index + `0x101F00` GBK list).

## Table layout (reader + packer)

Index base `0x100000`, 485 entries × 16 bytes:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 8 | lowercase ASCII pinyin, NUL-padded |
| 8 | 2 | `start` — LE index into GBK list |
| 10 | 2 | `count` — LE candidate count |
| 12 | 4 | reserved (`0xFFFFFFFF`) |

GBK list base `0x101F00`: packed `uint16_t` GBK codes (big-endian byte order as stored: hi, lo).

If OEM bytes differ, flash `docs/font/uvk1_pinyin.bin` to `0x100000` (generator: `pack_uvk1_pinyin.py`).

## UX

- **ChName:** `F` cycles abc → ABC → Numeric → Pinyin  
- **Compose:** `*` cycles English upper → lower → Numeric → Pinyin  
Pinyin: multi-tap letters → candidate row → select inserts 2-byte GB2312.  
Backspace deletes one pinyin letter, or one buffer codepoint (2 bytes if Chinese).

## Out of scope

Callsign / Yan ID / DTMF editors. Over-the-air peer Chinese rendering.
