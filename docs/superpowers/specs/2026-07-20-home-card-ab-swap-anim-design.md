# Home Card A/B Swap Animation

Date: 2026-07-20  
Status: Approved

## Goal

When the user manually switches A/B on the home card (dual RX / cross-band), animate the front card sliding up off-screen while the back card moves into the front position (~450ms). In MAIN ONLY, A/B switch is disabled with a short beep.

## Decisions (locked)

| Topic | Decision |
|-------|----------|
| Motion | Front card moves up and off screen; back card moves from peek offset to front |
| Duration | ~450ms (9 steps × 50ms) |
| Approach | Offset compositing with **lightweight** swap cards (border/name/freq only); no gauge soft-fill; no mid-frame `ClearRect` (would erase the other card). Final frame is always normal front + back peek. |
| Manual triggers | Long-press `2`, `F+2`, programmable `ACTION_OPT_A_B` — all via `COMMON_SwitchVFOs` |
| Auto RX main-follow | No animation (does not call `COMMON_SwitchVFOs`) |
| MAIN ONLY | Reject A/B switch; short beep (`BEEP_1KHZ_60MS_OPTIONAL`); no animation |
| Dual condition | Animation only when `DUAL_WATCH != OFF` or `CROSS_BAND_RX_TX != OFF` (same as back-peek) |
| RF timing | Apply VFO switch immediately at animation start; UI uses outgoing/incoming draw state |
| Interrupt | Key / PTT / leave main screen → cancel anim and draw final state |

## Behavior

### MAIN ONLY

```c
gEeprom.DUAL_WATCH == DUAL_WATCH_OFF &&
gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF
```

`COMMON_SwitchVFOs` returns early with short beep; no `TX_VFO` change.

### Manual switch (dual / cross-band)

1. Remember `outgoing = TX_VFO`.
2. Perform existing `COMMON_SwitchVFOs` side effects (toggle TX, DW/XB retarget, save, reconfigure).
3. Start home-card swap animation: outgoing slides up; incoming (`TX_VFO`) slides from peek (`BACK_OX`, `BACK_OY`) to front `(0,0)`.
4. ~45 × 10ms steps (or equivalent); linear interpolation.
5. Draw order each frame: clear → incoming (lower) → outgoing (upper).
6. On finish or cancel: clear anim flags; normal `UI_DisplayHomeCard`.

### Visual notes

- 1bpp: “fade” = clip as card moves off top; no alpha.
- Status-line text that would leave the status band is clipped.
- Channel name may use small-font path while offset ≠ 0 if line-based bold print cannot offset cleanly.

## Implementation touchpoints

| Area | File | Change |
|------|------|--------|
| MAIN ONLY gate + anim request | `App/app/common.c` | Early reject + beep; after switch call `UI_HomeCard_StartVfoSwapAnim` |
| Anim state / draw / 10ms | `App/ui/home_card.c` / `.h` | Offset draw, start/cancel/timeslice APIs |
| 10ms pump | `App/app/app.c` | Call home-card 10ms while animating → `gUpdateDisplay` |
| Cancel | `home_card` / key paths | Cancel if not on main, or on new input during anim |

## Acceptance

1. Dual/cross-band: long-press `2` / `F+2` / side A/B → front slides up, back becomes front (~450ms).
2. MAIN ONLY: those actions beep and do not change A/B.
3. RX auto main-follow still flips front instantly (no anim).
4. Mid-anim key/PTT/leave main → final layout immediately.

## Out of scope

- Classic (non-home-card) UI
- Changing dual-watch RF timing
- Animation on auto RX follow
