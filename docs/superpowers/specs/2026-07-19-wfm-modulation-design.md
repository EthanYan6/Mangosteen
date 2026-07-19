# WFM Modulation Mode Design

Date: 2026-07-19  
Status: Approved for implementation planning

## Goal

Add a VFO modulation mode `WFM` so broadcast FM (64–108 MHz) can be received on the main screen via the BK1080 chip, stored per channel, without the legacy standalone FM-radio UI.

## Decisions

| Topic | Choice |
|-------|--------|
| Integration style | VFO modulation (`MODULATION_WFM`), not separate `gFmRadioMode` UI |
| Frequency range | 64.0 – 108.0 MHz inclusive |
| Auto mode on enter range | Set modulation to WFM once when frequency enters/lands in range |
| Manual override | User may freely change Mode (FM/AM/USB/WFM/…) while in range; do not re-force WFM on further tuning inside the band |
| Auto mode on leave range | If still WFM, switch back to FM; if user already chose another mode, keep it |
| Manual WFM outside range | Allowed; no reject/clamp |
| Transmit | Forbidden for any mode when TX frequency is in 64–108 MHz; WFM never transmits |
| Legacy FM UI | Remove; keep BK1080 driver only for WFM RX |

## Architecture

### Mode enum

- Add `MODULATION_WFM` to `ModulationMode_t` (before `MODULATION_UKNOWN`).
- Add display string `"WFM"` to `gModulationStr[]`.
- Persist via existing channel/VFO modulation nibble (4-bit); no EEPROM layout change.
- Mode menu and F-key demod cycle include WFM automatically.

### Chip roles

| State | BK1080 | BK4819 |
|-------|--------|--------|
| WFM RX | Init + tune + audio | No NFM demod / skip CTCSS-compander path; reuse existing mute/LNA cooperation patterns from legacy FM |
| Non-WFM | Power down (`Init0`) | Normal `RADIO_SetupRegisters` / `RADIO_SetModulation` |

Unit conversion:

- VFO frequency unit: 0.01 kHz (`6400000` = 64.0 MHz, `10800000` = 108.0 MHz)
- BK1080 unit: 0.1 MHz (`640` … `1080`)
- `bk1080_freq = vfo_freq / 100000`

### Auto-mode helper

Introduce a single helper (name illustrative): `RADIO_ApplyWfmAutoMode(prev_freq, new_freq)` (or equivalent).

Behavior:

1. **Enter band**: `prev` outside `[64,108]` (or unknown) and `new` inside → set `Modulation = WFM`, request RX reconfigure.
2. **Leave band**: `prev` inside and `new` outside and current mode is WFM → set `Modulation = FM`, request RX reconfigure.
3. **Stay inside / stay outside**: do not change modulation (preserves user override).

Call sites:

- Frequency keypad entry (main VFO)
- Step up/down / frequency change paths that update VFO RX frequency

Do **not** auto-override modulation when loading a memory channel: a saved Mode (including non-WFM inside 64–108) wins.

### RX setup

In / beside `RADIO_SetupRegisters()`:

- If `Modulation == WFM`: configure BK1080 for current VFO frequency; open audio path; skip BK4819 NFM/CTCSS/compander setup used for `MODULATION_FM`.
- Else: ensure BK1080 is off; existing BK4819 path unchanged.

### TX forbid

- `TX_freq_check()`: fail for frequencies in `[64.0, 108.0]` MHz for all modes.
- WFM treated like other non-transmit demod modes (additional guard in TX prepare path).
- Existing UI TX-lock / inhibit indicators continue to apply.

### Frequency validation (BK4819 dead zone)

Today VFO entry / `RX_freq_check` reject or snap away from the BK4819 gap that overlaps broadcast FM.

- When operating as WFM (or when accepting input that will auto-enter WFM), allow **64–108 MHz**.
- Outside WFM, keep existing BK4819 limits.

### Build / packaging

- Split feature flag so BK1080 driver can build without legacy FM app:
  - Keep or introduce driver-only enable (e.g. `ENABLE_BK1080` / adjust `ENABLE_FMRADIO` to mean driver-only if cleaner).
  - Stop compiling `app/fm.c`, `ui/fmradio.c` and remove menu/key entry points to `DISPLAY_FM` / `ACTION_FM`.
- Fusion (and any preset that should support WFM) enables the BK1080 driver.

## Data flow (happy path)

```
User enters 101.7 MHz on main VFO
  → frequency accepted (WFM band allowed)
  → enter-band detect → Modulation = WFM
  → RADIO_SetupRegisters / WFM branch
  → BK1080 tune 1017 (0.1 MHz units), audio on
  → channel may be saved with Modulation = WFM

User changes Mode to USB while still on 101.7
  → Modulation = USB (allowed)
  → BK1080 off, BK4819 USB path (may be weak/unusable on this freq; user choice)

User steps to 102.0
  → still inside band → keep USB (no re-auto WFM)

User steps above 108.0 while on WFM
  → leave-band detect → Modulation = FM
  → BK1080 off, BK4819 FM path
```

## Edge cases

| Case | Behavior |
|------|----------|
| Exactly 64.0 or 108.0 | Inside band; enter → WFM |
| Cross 63.x → 64.0 | Enter → WFM |
| Cross 108.0 → above | Leave; if WFM → FM |
| Inside band, user chose USB, then step | Keep USB |
| Leave then re-enter band | Auto WFM again |
| Manual WFM outside band | Allowed; no clamp |
| PTT in 64–108 | Always denied |
| Save/load MR with WFM | Persist modulation nibble; RX setup follows mode |
| Dual watch | Active RX VFO WFM uses BK1080; switching to non-WFM VFO powers BK1080 down |
| Legacy FM screen/menu | Removed |

## Acceptance tests

1. Mode menu / F-cycle shows `WFM`; value saves to channel.
2. Entering 64–108 MHz auto-selects WFM and receives broadcast audio via BK1080.
3. Inside band, user can switch to FM/USB and selection sticks across further tuning inside the band.
4. Leaving band while on WFM auto-returns to FM and BK4819.
5. PTT denied for any mode on 64–108 MHz.
6. Firmware has no standalone FM-radio UI; BK1080 driver is present for WFM.

## Non-goals

- Restoring the legacy `DISPLAY_FM` application and FM channel memory UI.
- Using BK4819 wide-FM demod for 64–108 (hardware dead zone / wrong chip).
- Allowing transmit on broadcast FM band.

## Primary touch points

- `App/radio.h`, `App/radio.c` — enum, strings, RX setup, auto helper
- `App/frequencies.c` / `.h` — WFM range helpers, `RX_freq_check` / `TX_freq_check`
- `App/app/main.c`, `App/app/app.c` — frequency entry / step hooks
- `App/app/action.c`, `App/app/menu.c`, `App/ui/menu.c` — mode cycle / labels; remove FM UI entry
- `App/CMakeLists.txt`, `CMakePresets.json` — driver-only BK1080 build
- Remove or gate: `App/app/fm.c`, `App/ui/fmradio.c` and call sites behind legacy FM UI
