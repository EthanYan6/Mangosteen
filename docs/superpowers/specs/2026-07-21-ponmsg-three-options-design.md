# POnMsg Three Options Design

**Date:** 2026-07-21  
**Status:** Approved for planning  
**Approach:** A — slim enum + embedded mangosteen bitmap

## Goal

Replace the existing `POnMsg` (power-on display) menu with exactly three choices:

| Value | Label | Behavior |
|------:|-------|----------|
| 0 | `None` | Skip boot screen and boot wait; no power-on beep; go straight to main UI |
| 1 | `Default` | Show embedded mangosteen bitmap, then `Mangosteen <VERSION_STRING_2>` and `BD1AHN`; play power-on beep; keep ~2.55s boot wait (key press skips) |
| 2 | `Logo` | Show custom 128×64 bitmap from external flash; play power-on beep; same boot wait |

## Background

Today (with `ENABLE_FEAT_F4HWN` + `ENABLE_FEAT_F4HWN_LOGO`) `gSubMenu_PONMSG` is roughly:

`ALL`, `SOUND`, `MESSAGE`, `VOLTAGE`, `LOGO`, `NONE`

Welcome rendering and backlight beep branch on those modes in `ui/welcome.c`, `driver/backlight.c`, and `main.c`. Custom logos already live in PY25Q16 at `0x011000` (8-byte header + 1024-byte ST7565 bitmap).

## Data model

### Enum (`settings.h`)

```c
enum POWER_OnDisplayMode_t {
    POWER_ON_DISPLAY_MODE_NONE = 0,
    POWER_ON_DISPLAY_MODE_DEFAULT,
    POWER_ON_DISPLAY_MODE_LOGO,
};
```

Menu strings (`ui/menu.c`): `"NONE"`, `"DEFAULT"`, `"LOGO"`.  
`menu.h` array size becomes 3 (when LOGO feature is enabled; if LOGO compile flag is off, still expose None/Default only — prefer keeping Logo behind existing `ENABLE_FEAT_F4HWN_LOGO` so builds without logo flash support stay coherent).

**Decision:** Require `ENABLE_FEAT_F4HWN_LOGO` for the three-option menu as specified (None / Default / Logo). Builds without the logo feature keep a two-option menu: None / Default only.

### EEPROM

- Same storage byte as today (`Data[7]` at the existing settings block).
- On load: accept only `0 .. N-1` where `N` is the active submenu size; otherwise fall back to `POWER_ON_DISPLAY_MODE_DEFAULT`.
- Factory / invalid default: `Default` (not Voltage).

Old values (`ALL`/`SOUND`/`MESSAGE`/`VOLTAGE`/`old LOGO`/`old NONE` under previous numbering) are treated as invalid and map to `Default`. No attempt to remap old ordinals — enum renumbering makes remapping unreliable.

## UI behavior

### None

- `UI_DisplayWelcome`: clear / blank path, return immediately.
- `main.c`: do **not** enter the `boot_counter_10ms` wait loop.
- `BACKLIGHT_Sound`: do **not** play power-on beeps.

### Default

Layout on 128×64 monochrome LCD:

1. Centered embedded mangosteen bitmap (~48×48), derived from the uploaded reference photo (dithered to 1-bit).
2. Centered small text: `Mangosteen <VERSION_STRING_2>` (e.g. `Mangosteen v4.2`).
3. Next line centered: `BD1AHN`.

Bitmap is a `const uint8_t` array in firmware (e.g. `bitmaps.c` or a dedicated source file), drawn into the framebuffer — not read from flash.

Power-on beep: yes. Boot wait: yes (~2.55s, skippable by key).

### Logo

- Existing `UI_LoadLogo` / flash layout unchanged.
- Full-screen custom image.
- Power-on beep: yes. Boot wait: yes.
- Empty / unprogrammed logo sector: unchanged (may show blank/garbage); no new validation.

## Code touch points

| Area | Change |
|------|--------|
| `App/settings.h` | New three-value enum |
| `App/settings.c` | Load clamp + default → Default |
| `App/ui/menu.c`, `menu.h` | `gSubMenu_PONMSG` size/content |
| `App/ui/welcome.c` | Render Default; drop ALL/MESSAGE/VOLTAGE/SOUND branches |
| `App/driver/backlight.c` | Beep on Default and Logo only |
| `App/main.c` | Boot wait when mode ≠ None |
| Bitmap sources | Add mangosteen 1-bit asset |
| `docs/js/lang.js` (and related flasher copy) | Point users to menu `Logo` instead of old “自定义 / Custom” wording where needed |

## Out of scope

- Changing screensaver / `SET_SAV` matrix / logo-saver behavior.
- New logo flash protocol or header format.
- Color or grayscale LCD support.
- Remapping legacy EEPROM ordinals to the closest old semantic.

## Testing

1. Set each of None / Default / Logo, power cycle, confirm persistence.
2. None: no welcome art, no beep, immediate main screen.
3. Default: mangosteen + two text lines + beep + wait (key skips wait).
4. Logo: flash custom image + beep + wait.
5. Write EEPROM byte ≥ 3 (or leftover old value), reboot → behaves as Default.
6. Build with and without `ENABLE_FEAT_F4HWN_LOGO` if both are still supported presets.

## Decisions log

- Approach A: embedded Default bitmap; Logo remains flash custom image.
- Power-on beep for both Default and Logo (user choice A).
- Visual preview approved for stacked icon + two text lines on Default.
