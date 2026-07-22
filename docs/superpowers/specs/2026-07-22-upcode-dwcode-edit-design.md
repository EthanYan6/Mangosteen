# UPCode / DWCode Menu Editing Design

**Date:** 2026-07-22  
**Status:** Approved for implementation planning  
**Scope:** Enable in-radio editing of DTMF UPCode and DWCode, following the ChName text-edit interaction pattern with a restricted DTMF character set.

## Problem

UPCode and DWCode appear in the settings menu and can display their current values, but pressing MENU does nothing. `MENU_Key_MENU` returns early for `MENU_UPCODE` / `MENU_DWCODE`, and there is no text-edit or save path for these items.

## Goal

Allow the user to edit UPCode and DWCode on the radio with the same enter / cursor / confirm flow as ChName (`MENU_MEM_NAME`), while restricting editable characters to valid DTMF codes.

Out of scope for this change:

- ANI ID editing (still blocked)
- Kill / Revive code editing
- CPS / serial programming changes

## Approach

Extend the existing ChName-style text-edit infrastructure (`MENU_IsTextEditMenuItemId`, `edit[]` / `edit_index`, confirmation, `UI_MENU_DrawEditCard`) rather than adding a separate DTMF editor state machine.

## Interaction Flow

1. Highlight **UPCode** or **DWCode** in the menu list.
2. Press **MENU** → enter submenu (remove the current early `return`).
3. Press **MENU** again → enter cursor edit mode (`edit_index = 0`), loading the current EEPROM value into `edit[]`, padded with spaces to length 16.
4. While editing:
   - **UP / DOWN**: cycle the character under the cursor through the allowed set only.
   - **0–9**: write that digit into the current position (direct, no multi-tap).
   - **\***: write `*` into the current position.
   - **#** (F / `#` key, **short press**): write `#` into the current position.
   - **MENU**: advance cursor; long-press MENU jumps to end-of-edit like ChName.
5. If the buffer differs from the original → confirmation prompt (same style as ChName / ChSave).
6. Confirm → validate and save; cancel / unchanged → exit without writing.

No uppercase toggle and no multi-tap letter IME for these two menu items.

## Character Set

Allowed characters while editing:

| Source | Characters |
|--------|------------|
| Nav cycle set | ` ` (space), `0`–`9`, `A`–`D`, `*`, `#` |
| Digit keys | `0`–`9` only |
| `*` key | `*` only |
| `#` key | `#` only |

- Space exists only to clear a position / shorten the code; it is trimmed on save and is not a stored DTMF symbol.
- Any other character must be impossible to enter in this edit mode.
- Max length: **16** (matches `gEeprom.DTMF_UP_CODE` / `DTMF_DOWN_CODE`).

Nav wrap order (recommended):

```text
[space] 0 1 2 3 4 5 6 7 8 9 A B C D * #
```

If the current cell holds an unexpected character when edit starts, the first UP/DOWN press snaps it onto the nearest allowed character in the cycle direction.

## Validation and Save

On accept:

1. Copy `edit[]` into a temp buffer of size 16.
2. Trim trailing spaces to `\0` (and treat interior trailing padding the same way as ChName trailing-space cleanup: trailing spaces become terminators).
3. Call `DTMF_ValidateCodes(buf, 16)`.
4. If invalid or empty (`DTMF_ValidateCodes` already rejects leading `\0` / `0xFF`):
   - Do not overwrite EEPROM / `gEeprom`.
   - Exit edit mode (keep previous value).
5. If valid:
   - Copy into `gEeprom.DTMF_UP_CODE` or `gEeprom.DTMF_DOWN_CODE`.
   - Write flash:
     - UP: `0x00A0F8 + 0x18` (16 bytes)
     - DW: `0x00A0F8 + 0x28` (16 bytes)
   - Also persist these fields inside `SETTINGS_SaveSettings()` so a later general settings save does not lose them (today Load reads them but Save does not write them).

## UI

- Custom menu layout: when `edit_index >= 0` and current id is `MENU_UPCODE` or `MENU_DWCODE`, draw `UI_MENU_DrawEditCard` with `text_edit = true`, `edit_len = 16`.
- Small font pitch already fits 16 characters in the edit card width.
- Do not show ABC/abc case hint for these items (no case toggle). If the shared draw helper always prints the hint, suppress it when the current menu is UPCode/DWCode, or leave the hint unused (prefer suppress for clarity).

## Code Touch Points

| File | Change |
|------|--------|
| `App/app/menu.c` | Remove early return for UP/DW; add to text-edit menu id set; max len 16; load/save branches; digit/`*`/`#`/UP-DOWN restricted handlers; confirmation list includes UP/DW |
| `App/ui/menu.c` | Wire edit card for UP/DW; optional hide case hint |
| `App/settings.c` | Write UP/DW (and ideally existing ANI/Kill/Revive block consistency) in `SETTINGS_SaveSettings` |

Reuse `DTMF_ValidateCodes` from `App/app/dtmf.c` — do not duplicate validation rules.

## Key Handling Detail

When `MENU_IsTextEditMenuItemId` is true for UP/DW and `edit_index >= 0`:

- **0–9**: set `edit[edit_index] = '0' + key`; reset multi-tap state; refresh display. Do not use `char_map`.
- **\*** (short press): set `edit[edit_index] = '*'` (never `-`).
- **F / #** (short press): set `edit[edit_index] = '#'`; do **not** toggle `edit_is_uppercase` for UP/DW (unlike ChName, which uses short F for case and long F for `#`).
- **UP/DOWN**: index into the fixed allowed string and wrap; do not walk ASCII 32–126.
- Other keys: unchanged relative to ChName (EXIT backs out, etc.).

ChName / Yan ID / Messenger callsign keep their current unrestricted (or existing) behavior.

## Testing

Manual on device / simulator:

1. Open UPCode → MENU → MENU → edit starts with current value (default often `12345`).
2. UP/DOWN only yields space / 0–9 / A–D / * / #.
3. Digit keys overwrite current cell with 0–9.
4. `*` and `#` keys overwrite with `*` and `#`.
5. Shorten code with spaces → confirm → value shortens and still validates.
6. Clear entire code to spaces → confirm → value unchanged (reject empty).
7. Valid edit persists across power cycle.
8. DWCode same behavior independently.
9. ChName editing still supports letters / case toggle as before.

## Non-Goals / Explicit Non-Changes

- Do not enable ANI ID editing in this task.
- Do not change DTMF TX timing menus.
- Do not change default strings (`12345` / `54321`) except via user edit.
