# SysInf Author Other Firmware Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a last SysInf page showing author’s other firmware (叮咚鸡 / Dondji) based on saved `gUiLanguage`.

**Architecture:** Extend existing `MENU_VOL` pagination: bump `vol_max` by 1, render one more page after MEMORY/QR using `SUBV()` for EN/CN strings.

**Tech Stack:** C firmware (`App/ui/menu.c`, `App/app/menu.c`), GB2312 UI strings, `gUiLanguage` / `SUBV`.

**Spec:** `docs/superpowers/specs/2026-07-24-sysinf-author-other-fw-design.md`

---

## File map

| File | Change |
|---|---|
| `App/app/menu.c` | `MENU_GetLimits(MENU_VOL)`: `vol_max += 1` under `ENABLE_FEAT_F4HWN` |
| `App/ui/menu.c` | `MENU_VOL` display: last-page `SUBV("Other Firmware\nDondji", CN_GB2312)` |

No new files. No unit tests in this firmware tree for menu UI — verify by build + on-device scroll.

---

### Task 1: Bump SysInf page max

**Files:**
- Modify: `App/app/menu.c` (`MENU_GetLimits` / `MENU_VOL`)

- [ ] **Step 1: Add last-page slot**

After the QRCODE `#ifdef` block inside `case MENU_VOL`, add:

```c
#ifdef ENABLE_FEAT_F4HWN
    vol_max += 1; /* author other firmware (last page) */
#endif
```

Update the comment above `vol_max` to mention `+1 author other FW`.

- [ ] **Step 2: Commit**

```bash
git add App/app/menu.c
git commit -m "Bump SysInf page count for author-other-firmware page."
```

---

### Task 2: Render author other firmware page

**Files:**
- Modify: `App/ui/menu.c` (`case MENU_VOL`)

- [ ] **Step 1: Append page after QR**

Replace the trailing `break;` after `#endif` (QRCODE) inside `case MENU_VOL` with:

```c
#ifdef ENABLE_FEAT_F4HWN
            if (page == p++) {
                /* Saved UI language: CN -> 作者其它固件/叮咚鸡, EN -> Other Firmware/Dondji */
                strcpy(String, SUBV(
                    "Other Firmware\nDondji",
                    "\xD7\xF7\xD5\xDF\xC6\xE4\xCB\xFC\xB9\xCC\xBC\xFE\n"
                    "\xB6\xA3\xDF\xCB\xBC\xA6"));
                break;
            }
#endif
            break;
```

Also update the page-order comment at the top of `case MENU_VOL` to list the new last page.

- [ ] **Step 2: Build firmware (sanity)**

Use the project’s usual CMake preset (e.g. from `CMakePresets.json`). Expect: compile succeeds.

- [ ] **Step 3: Commit**

```bash
git add App/ui/menu.c
git commit -m "Show author other firmware on last SysInf page."
```

---

### Task 3: Manual acceptance

- [ ] Enter 系统信息 / SysInf, scroll to last page
- [ ] Language = 中文 → `作者其它固件` / `叮咚鸡`
- [ ] Language = English → `Other Firmware` / `Dondji`
- [ ] Prior SysInf pages unchanged

---

## Spec coverage

| Spec requirement | Task |
|---|---|
| Last page | Task 1 + 2 |
| CN / EN strings via saved language | Task 2 (`SUBV` / `gUiLanguage`) |
| F4HWN-only | Both tasks gated `#ifdef ENABLE_FEAT_F4HWN` |
| No EEPROM / no new menu id | — |
