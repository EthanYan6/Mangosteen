# SysInf: Author Other Firmware Page

## Goal

Add one final page to **系统信息** (`MENU_VOL` / SysInf) that credits the author’s other firmware:

| Language (`gUiLanguage`) | Title | Body |
|---|---|---|
| Chinese (`UI_LANGUAGE_CN`) | 作者其它固件 | 叮咚鸡 |
| English (otherwise) | Other Firmware | Dondji |

Language must follow the **saved UI language** (`gUiLanguage`), same as other menu i18n via `SUBV()` / `gUiLanguage == UI_LANGUAGE_CN`.

## Scope

- Only when `ENABLE_FEAT_F4HWN` (multi-page SysInf already requires it).
- Page order: identity → BUILD → BATTERY → [MEMORY] → [QR…] → **Author other FW** (always last).
- No new menu item, no EEPROM changes, no QR/link.

## Approach

Extend existing SysInf pagination (Approach A):

1. **`App/app/menu.c` — `MENU_GetLimits(MENU_VOL)`**  
   After existing MEM/QR increments: `vol_max += 1`.

2. **`App/ui/menu.c` — `MENU_VOL` display**  
   After QR (or after BATTERY/MEMORY if QR off), when `page == p++`:
   ```c
   strcpy(String, SUBV("Other Firmware\nDondji",
                       "\x...\x..." /* 作者其它固件\n叮咚鸡 GB2312 */));
   ```
   Classic layout uses the normal multi-line `String` path; custom list layout already consumes `String` as the card value.

## Non-goals

- Listing more firmwares, URLs, or actionable open/download.
- Changing page order of existing SysInf pages.
- Regenerating full `_gen_menu_cn.py` tables (inline GB2312 escape is enough for this one page).

## Acceptance

- Enter 系统信息 / SysInf, scroll to last page.
- UI language Chinese → title 作者其它固件, body 叮咚鸡.
- UI language English → title Other Firmware, body Dondji.
- Switching language in settings and re-opening SysInf shows the matching strings (uses saved `gUiLanguage`).
- Existing SysInf pages unchanged in content and relative order.
