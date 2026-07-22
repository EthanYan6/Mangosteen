# UPCode / DWCode Edit Implementation Plan

> **For agentic workers:** Implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable ChName-style in-menu editing of DTMF UPCode and DWCode with a restricted character set.

**Architecture:** Extend existing text-edit menu path; restrict keys for UP/DW only; validate with `DTMF_ValidateCodes`; persist via direct EEPROM write and `SETTINGS_SaveSettings`.

**Tech Stack:** C firmware, menu UI, PY25Q16 EEPROM, existing DTMF helpers.

**Spec:** `docs/superpowers/specs/2026-07-22-upcode-dwcode-edit-design.md`

---

### Task 1: Persist DTMF UP/DW in SaveSettings

**Files:**
- Modify: `App/settings.c`

- [x] After existing settings blocks in `SETTINGS_SaveSettings`, write:
  - `gEeprom.DTMF_UP_CODE` → `0x00A0F8 + 0x18` (16 bytes)
  - `gEeprom.DTMF_DOWN_CODE` → `0x00A0F8 + 0x28` (16 bytes)

### Task 2: Menu accept + text-edit plumbing

**Files:**
- Modify: `App/app/menu.c`

- [x] `MENU_IsTextEditMenuItemId`: include `MENU_UPCODE`, `MENU_DWCODE`
- [x] `MENU_TextEditMaxLen`: return 16 for those ids
- [x] Add `MENU_IsDtmfCodeEdit()` helper (UP/DW only)
- [x] Remove early `return` for UP/DW in `MENU_Key_MENU` (keep ANI ID blocked)
- [x] Enter-edit load from `gEeprom.DTMF_UP_CODE` / `DTMF_DOWN_CODE`
- [x] Add confirmation list membership for UP/DW
- [x] `MENU_AcceptSetting`: trim, validate, copy to `gEeprom`, `gRequestSaveSettings = 1` (or direct write + save)

### Task 3: Restricted key handlers

**Files:**
- Modify: `App/app/menu.c`

- [x] Digits: direct `'0'+n` when DTMF code edit
- [x] STAR short: `*` only when DTMF code edit
- [x] F short: `#` only (no case toggle) when DTMF code edit
- [x] UP/DOWN: cycle `" 0123456789ABCD*#"` only when DTMF code edit

### Task 4: UI edit card

**Files:**
- Modify: `App/ui/menu.c`

- [x] Wire `DrawEditCard` for UP/DW with `edit_len = 16`
- [x] Suppress ABC/abc hint for UP/DW

### Task 5: Verify build

- [x] Syntax-check `menu.c` / `ui/menu.c` / `settings.c` with ARM GCC (`-fsyntax-only`); full Docker build unavailable (daemon not running)