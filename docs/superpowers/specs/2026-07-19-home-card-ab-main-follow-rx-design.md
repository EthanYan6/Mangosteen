# Home Card A/B Indicator + Main VFO Follows RX

Date: 2026-07-19  
Status: Approved

## Goal

On the home card UI:

1. Show which VFO is on the front card (`A` or `B`) to the left of the channel number.
2. When both VFOs can receive, the VFO that receives a signal becomes the front card, the main VFO, and the TX VFO — and stays there until the other VFO receives or the user long-presses `2`.

## Decisions (locked)

| Topic | Decision |
|-------|----------|
| A/B placement | Left of channel number on the front card |
| Sticky model | No separate sticky variable; front card always draws `TX_VFO` |
| Main / TX / front | Always the same VFO: whoever is on top is main and TX |
| Auto-switch modes | Only when dual RX is possible: **DUAL RX RESPOND** and **MAIN TX DUAL RX** |
| Non-auto modes | **MAIN ONLY** and **CROSS BAND**: do not auto-switch main on RX; still show A/B |
| TX while sticky | TX uses current main (`TX_VFO`); receiving updates main so TX follows the top card |
| Manual switch | Long-press `2` (`COMMON_SwitchVFOs`) remains the manual A/B toggle |
| Radio DW timing | Do not rewrite dual-watch scan/resume timing; only align main VFO when listening starts |

## RxMode mapping

Existing menu `MENU_TDR` / `gSubMenu_RXMode`:

| Index | Label | `DUAL_WATCH` | `CROSS_BAND_RX_TX` | Auto main-follow |
|-------|-------|--------------|--------------------|------------------|
| 0 | MAIN ONLY | off | off | No |
| 1 | DUAL RX RESPOND | on | off | Yes |
| 2 | CROSS BAND | off | on | No |
| 3 | MAIN TX DUAL RX | on | on | Yes |

Condition for auto-follow:

```c
gEeprom.DUAL_WATCH != DUAL_WATCH_OFF
```

(covers indices 1 and 3).

## Behavior

### Display

In `HC_DrawFront` (today draws channel label at x≈68, y=3):

- Draw `A` or `B` for `vfo_num` immediately left of the channel number.
- Keep battery / percent positions stable; shift channel number right only if needed for spacing (prefer compact `"A"` + existing number at a slightly adjusted x).

### Front / back card selection

Unchanged structurally:

- `front = gEeprom.TX_VFO & 1`
- `back = front ^ 1`
- Back peek only when DW or XB is enabled

Because main follows RX, the front card content follows automatically.

### Auto main-follow on RX

Hook: `APP_StartListening` (where dual-watch hold already begins), when:

1. `gScanStateDir == SCAN_OFF`
2. `gEeprom.DUAL_WATCH != DUAL_WATCH_OFF`
3. Incoming VFO (`gEeprom.RX_VFO`) differs from `gEeprom.TX_VFO`

Then perform a main-VFO align equivalent to a targeted switch (same side effects as `COMMON_SwitchVFOs` for DW/XB bookkeeping, but set absolute target instead of toggle):

1. `gEeprom.TX_VFO = gEeprom.RX_VFO`
2. If `CROSS_BAND_RX_TX != OFF`: `CROSS_BAND_RX_TX = TX_VFO + 1`
3. If `DUAL_WATCH != OFF`: `DUAL_WATCH = TX_VFO + 1`
4. `gRequestSaveSettings = 1`
5. `gFlagReconfigureVfos = true` (as needed for consistent pointers)
6. Refresh home card / status (`gUpdateDisplay` / `gUpdateStatus` as appropriate)

After this, top card, main, and TX remain on that VFO until:

- the other VFO receives (same hook runs again), or
- user long-presses `2`.

### Long-press 2

Existing `COMMON_SwitchVFOs` behavior is sufficient: toggles `TX_VFO`, retargets DW/XB, saves, shows main screen. Front card follows because it draws `TX_VFO`.

### What not to change

- Do not clear/extend `gRxVfoIsActive` lifecycle for UI stickiness.
- Do not change dual-watch countdown lengths.
- Do not invent a parallel `gHomeCardFrontVfo` unless a later bug proves `TX_VFO` alone is insufficient.

## Implementation touchpoints

| Area | File | Change |
|------|------|--------|
| A/B glyph | `App/ui/home_card.c` | Draw A/B left of channel label in `HC_DrawFront` |
| Main-follow | `App/app/app.c` (`APP_StartListening`) | When DW enabled and RX VFO ≠ TX VFO, align TX/main to RX |
| Optional helper | `App/app/common.c` / `.h` | e.g. `COMMON_SetMainVfo(uint8_t vfo)` shared by switch + follow (optional refactor) |

## Acceptance checks

1. Channel number shows `A`/`B` prefix (or adjacent letter) matching the front VFO.
2. DUAL RX RESPOND: signal on non-main VFO → that VFO moves to front; A/B updates; PTT transmits on that VFO; stays after RX ends.
3. MAIN TX DUAL RX: same as (2).
4. MAIN ONLY / CROSS BAND: RX on either path does not auto-flip main; A/B still reflects current main.
5. Long-press `2` flips front/main/TX immediately.
6. Next RX on the other VFO flips again.

## Out of scope

- HTML brainstorm mockups (optional later sync)
- Classic (non-home-card) main UI changes
- Side-key programmable A/B action changes beyond existing `COMMON_SwitchVFOs` path
