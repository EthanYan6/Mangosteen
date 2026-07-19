# Design: Generic Centered Message Box

Date: 2026-07-19

## Goal

Add a reusable centered message box overlay for short feedback messages. First uses:

1. **TX Disabled!** — shown when the current frequency (or other TX gate) forbids transmit
2. **Key Locked** — shown when the keypad is locked and the user presses a blocked key

The box appears on **any screen** (main, menu, etc.) and auto-dismisses after ~2 seconds.

## English copy

| Use case | Text |
|----------|------|
| Transmit forbidden | `TX Disabled!` |
| Keypad locked key press | `Key Locked` |

Notes on rejected wording: `Tx disable!` is less idiomatic; `key clock` is incorrect (clock ≠ lock).

## API

In `ui/helper.c` / `ui/helper.h`:

```c
void UI_ShowMessageBox(const char *text);  // arm overlay for ~2s
void UI_DrawMessageBox(void);              // paint if countdown active
```

### State

| Symbol | Role |
|--------|------|
| `gMessageBoxText` | Pointer to static C string (or equivalent short buffer) |
| `gMessageBoxCountdown` | 500 ms ticks; start at `4` (= 2 s), same cadence as `gKeypadLocked` |

`UI_ShowMessageBox`:

1. Store text pointer
2. Set countdown to 4
3. Set `gUpdateDisplay = true`

Re-trigger while visible: replace text and reset countdown to 4.

## Visual design

- LCD: 128×64 ST7565; overlay drawn into `gFrameBuffer` (main area, not status bar)
- Centered rectangle ≈ 90×20 px (tune to fit longest string)
- Interior cleared (white), text black via `UI_PrintStringSmallNormal`, horizontally centered
- Border: top and left single-pixel; right and bottom **double-pixel** (second line offset by 1) for a “raised” look

ASCII sketch:

```
┌─────────────────────┐
│   TX Disabled!      ║
└═════════════════════╝
```

## Draw / blit integration

`GUI_DisplayScreen()` ends with `UI_DrawMessageBox()` so every screen type gets the overlay without per-screen duplication.

Countdown decrement lives next to the existing 500 ms tick that already handles `gKeypadLocked` (in `app/app.c`). When countdown reaches 0, set `gUpdateDisplay = true` so the box clears on the next paint.

## Trigger points

### Key Locked

Existing path in `app/app.c` when `gEeprom.KEY_LOCK` (or low-battery popup gate) blocks a key:

- Keep beep
- Call `UI_ShowMessageBox("Key Locked")`
- Prefer consolidating with `gKeypadLocked` where possible (either drive the overlay from that countdown, or keep a dedicated message-box countdown and stop drawing the old full-screen lock banners)

Classic full-screen strings (`"Long press #"`, `UI_DisplayUnlockKeyboard`) are superseded by the message box when this feature is active.

### TX Disabled

Existing path: `RADIO_PrepareTX()` → `RADIO_SetVfoState(VFO_STATE_TX_DISABLE)`.

- Keep VFO state / TX abort behavior unchanged
- On entering `VFO_STATE_TX_DISABLE`, call `UI_ShowMessageBox("TX Disabled!")`
- Home-card UI no longer depends on classic inline `VfoStateStr["TX DISABLE"]` for this feedback

## Out of scope

- Changing TX-frequency rules or key-lock policy
- EXIT-to-dismiss or animations
- Changing the low-battery modal (`UI_DisplayPopup("LOW BATTERY")`)

## Files likely touched

- `App/ui/helper.c`, `App/ui/helper.h` — draw + show API
- `App/ui/ui.c` — call `UI_DrawMessageBox` after screen paint
- `App/misc.c`, `App/misc.h` — countdown / text globals (if not kept local to helper)
- `App/app/app.c` — tick + key-lock trigger
- `App/radio.c` — TX-disable trigger
- Optionally `App/ui/main.c` / `App/ui/helper.c` — stop or gate old lock banners to avoid double messaging
