# Mangosteen Messenger Design

Date: 2026-07-19  
Reference: [GOGUFW-UV-K1-Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger)

## Goal

Port GOGUFW’s messaging feature set into Mangosteen so UV-K1 units running Mangosteen can exchange digital text with GOGUFW radios. UI, protocol, and storage layout stay aligned with GOGUFW. BEAM, AirCopy, and the Breakout game are disabled to free FSK resources and the `F+7` shortcut.

## Decisions (confirmed)

| Topic | Choice |
|---|---|
| Scope | Messenger + HEARD + Range Check + drafts + ACK/retry + assignable shortcuts |
| Out of scope | CALLTX, Survival Mode, FM Naming, CHIRP module |
| Interop | Must interoperate with GOGUFW (same `GGM2` packet format) |
| UI | Keep GOGUFW Messenger / HEARD / Range Check UI as-is |
| Integration approach | Approach 1: copy GOGUFW messenger modules; wire into Mangosteen build/runtime |
| Flash | Use GOGUFW private sector `0x012000` (4 KB) |
| BEAM / AirCopy UI | Disabled in all presets (`ENABLE_FEAT_F4HWN_BEAM=false`, `ENABLE_AIRCOPY=false`). Messenger still compiles `app/aircopy.c` FSK buffer helpers and patches `messenger_rf.c` so the FSK path builds under `ENABLE_MESSENGER` (AirCopy screens/menus stay off). |
| Game | Disabled (`ENABLE_FEAT_F4HWN_GAME=false`); `F+7` becomes HEARD (GOGUFW-consistent) |

## Architecture

```
┌─────────────────────────────────────────┐
│ UI / Keys (GOGUFW-identical screens)      │
│  Messenger / HEARD / Range Check / T9     │
├─────────────────────────────────────────┤
│ messenger.c         state machine, ticks  │
│ messenger_ui.c / messenger_t9.c           │
├─────────────────────────────────────────┤
│ messenger_rf.c      FSK TX/RX, ACK/retry  │
│ messenger_packet.c  GGM2 codec (interop)  │
│ messenger_store.c   config @ 0x012000     │
├─────────────────────────────────────────┤
│ BK4819 FSK + PY25Q16                      │
│ (BEAM / AirCopy compile paths off)        │
└─────────────────────────────────────────┘
```

Mangosteen’s home-card main screen remains unchanged. Messenger-related screens are separate full-screen UIs; exiting them returns to the home screen. Status-line unread indication follows GOGUFW.

## Modules and files

### New files (ported from GOGUFW, keep behavior/UI)

| File | Role |
|---|---|
| `App/app/messenger.c/.h` | Entry, state machine, tick, open helpers |
| `App/app/messenger_ui.c/.h` | Inbox / Compose / Sent / Drafts / HEARD / Range Check UI |
| `App/app/messenger_t9.c/.h` | Shared T9 editor |
| `App/app/messenger_rf.c/.h` | FSK path, ACK/retry, boot-time RX sidecar |
| `App/app/messenger_packet.c/.h` | `GGM2` wire format (`MSG_PKT_MAGIC` `GGM2`, version 2) |
| `App/app/messenger_store.c/.h` | Config/drafts persistence at `0x012000` |

### Mangosteen integration touchpoints

| File | Change |
|---|---|
| `App/CMakeLists.txt` | Add GOGUFW-identical `enable_feature(ENABLE_MESSENGER …)` block for the six messenger source pairs |
| `CMakePresets.json` | Set `ENABLE_MESSENGER=true`; force `ENABLE_AIRCOPY=false`, `ENABLE_FEAT_F4HWN_BEAM=false`, `ENABLE_FEAT_F4HWN_GAME=false` on all presets |
| `App/driver/eeprom_compat.c` | Add mapping `0x012000` ↔ EEPROM alias `0x00E000` (same as GOGUFW; CHIRP module not required) |
| `App/app/app.c` | Init, 10 ms tick, FSK interrupt dispatch to `MSG_RF_*` |
| `App/app/action.c`, `App/settings.h` | Actions for Messenger / HEARD (and Range Check if GOGUFW exposes one) |
| `App/ui/*` (status, key routing, display enum as needed) | Unread icon, key routing matching GOGUFW |
| Menu tables | Messenger settings (callsign, ACK, beep, hop, RX enable, etc.) |

### Explicitly not ported

- CALLTX UI / actions / `F+9` call-tone feature (store struct may still contain `call_tone` / `call_vol` fields so `0x012000` layout stays byte-compatible with GOGUFW)
- Survival Mode
- FM Naming (`0x013000` sector unused)
- CHIRP driver/module for GOGUFW settings
- Game (`breakout`) remains in tree but disabled via preset

### FSK ownership

GOGUFW `messenger_rf.c` historically gates the real FSK TX/RX path on `ENABLE_AIRCOPY` and uses `g_FSK_Buffer` from `app/aircopy.c`. Product intent is “no AirCopy feature,” not “strip FSK plumbing.” Therefore: keep `ENABLE_AIRCOPY=false` (no `ui/aircopy.c`, no `DISPLAY_AIRCOPY`); when `ENABLE_MESSENGER` is on, still compile `app/aircopy.c` and widen the RF `#ifdef` to `ENABLE_MESSENGER`. BEAM remains fully off.

## Flash layout conflict check

| PY25Q16 sector | Mangosteen today | After this work |
|---|---|---|
| `0x011000` | Boot logo | Unchanged |
| `0x012000` | Unused | Messenger config/drafts (GOGUFW-compatible) |
| `0x013000` | Unused | Remains unused (FM Naming out of scope) |
| `0x1E0000` | Optional RXTX log | Unchanged; no overlap |

Conclusion: `0x012000` does not conflict with Mangosteen channel data, settings, logo, or RXTX log.

## Data flows

### Send

1. User composes via T9 (or draft).
2. `MSG_RF_SendText` builds a `GGM2` TEXT frame (`messenger_packet`).
3. Frame transmitted over BK4819 FSK.
4. Outbox entry marked `PENDING`.
5. ACK wait / retry per config → `ACKED` or `FAILED`; ACK sources recorded when present.

### Receive (including boot-time / home-screen sidecar)

1. FSK interrupt → `MSG_RF_OnRadioInterrupt`.
2. Parse `GGM2`.
3. TEXT: duplicate filter → Inbox (`unread`) → optional ACK reply → beep / unread status.
4. ACK: update matching Sent status and ACK source list.
5. PING/PONG: feed Range Check and HEARD.

Opening the Messenger UI is not required to receive messages.

### Shortcuts (GOGUFW-aligned)

| Key / action | Behavior |
|---|---|
| `F+MENU` (and assignable shortcut) | Open Messenger |
| `F+7` (and assignable shortcut) | Open HEARD (replaces former Game binding) |
| Inside Messenger | Inbox, Compose, Sent, Drafts, Reply, Delete, Resend, Range Check entry as in GOGUFW |

### Persistence

- Persisted at `0x012000`: callsign, drafts, feature toggles, `next_msg_id`, and other fields in `MSG_Config_t` as defined by GOGUFW store version in use.
- Inbox / Sent lists are RAM-only (power-loss clears them), matching GOGUFW.

## Error handling

| Case | Behavior |
|---|---|
| Bad CRC / magic | Drop frame |
| Duplicate TEXT (`from` + `id`) | Do not create a second Inbox entry; ACK path may still run |
| Busy channel / active FSK RX | Defer ACK |
| ACK timeout | Retry per config, then `FAILED` |
| Bad flash magic / unexpected version | Restore defaults and rewrite `0x012000` (follow GOGUFW migration rules already in `messenger_store.c`) |
| Inbox / Outbox full | Drop oldest (existing store behavior) |
| Invalid callsign chars | Sanitize before save |

## Testing

1. **Build**: Main preset builds cleanly with Messenger on and AirCopy / BEAM / Game off.
2. **UI smoke**: Compose → send → Sent status; navigate Inbox / Drafts / HEARD / Range Check; exit back to home cards.
3. **Interop**: Exchange TEXT with a GOGUFW radio; with ACK enabled, Sent shows ACK source(s).
4. **Sidecar RX**: Receive while on home cards; unread indication appears.
5. **Range Check**: Ping/Pong list updates with callsign / RSSI / voltage.
6. **Regression**: Home cards, FM radio integration, scan, and other Mangosteen features still work; no AirCopy / BEAM / Game entry points remain enabled.
7. **Flash**: Callsign/drafts survive power cycle; logo sector `0x011000` and channel memories untouched.

## Non-goals

- Redesigning Messenger UI to match home cards
- Protocol changes that break GOGUFW interop
- Porting CALLTX, Survival Mode, FM Naming, or CHIRP
- Keeping Game on an alternate key in this iteration

## Implementation notes

- Prefer copying the GOGUFW messenger sources at a pinned upstream revision (latest `main` at implementation time, record commit SHA in the PR/commit message) and applying only Mangosteen glue (CMake, presets, hooks).
- Feature flag is exactly `ENABLE_MESSENGER` (as in GOGUFW `App/CMakeLists.txt`).
- Attribute GOGUFW / F4HWN provenance in file headers where required by Apache-2.0.
- `eeprom_compat.c` is currently compiled when `ENABLE_AIRCOPY OR ENABLE_UART OR ENABLE_USB`. With AirCopy off, ensure UART and/or USB remain enabled so EEPROM mapping still builds, or extend that condition to `OR ENABLE_MESSENGER` if needed.
- Document in README briefly: Messenger enabled, interop with GOGUFW, Game/`F+7` → HEARD, AirCopy/BEAM off.
