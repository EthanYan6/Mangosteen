# Mangosteen Messenger Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port GOGUFW Messenger (Inbox/Compose/Sent/Drafts/ACK/HEARD/Range Check) into Mangosteen with wire/UI/storage parity for GOGUFW interop; disable Game/BEAM/AirCopy UI; map `F+7` to HEARD and `F+MENU` to Messenger.

**Architecture:** Copy the six GOGUFW `messenger_*` modules at a pinned revision, gate with `ENABLE_MESSENGER`, and wire the same hooks GOGUFW uses (`app.c` tick/IRQ, `main.c` keys, `action.c` shortcuts, menus, `DISPLAY_MESSENGER`). Keep Mangosteen home-card UI unchanged. FSK buffer helpers come from `app/aircopy.c` **without** enabling AirCopy UI.

**Tech Stack:** C firmware (PY32F071 + BK4819), CMake presets, Docker `./compile-with-docker.sh Custom`, PY25Q16 flash sector `0x012000`.

**Spec:** `docs/superpowers/specs/2026-07-19-messenger-design.md`

**Upstream pin:** `Gogu-Qs/GOGUFW-UV-K1-Messenger` @ `e334a5c6fff4bbecb328c49c009e4a58311593b0`

---

## Spec refinement (required for a working port)

GOGUFW `messenger_rf.c` wraps the real FSK TX/RX path in `#ifdef ENABLE_AIRCOPY` and uses `g_FSK_Buffer` / `BK4819_SetupAircopy()` from `app/aircopy.*`. Turning `ENABLE_AIRCOPY` off without further changes would compile Messenger but leave RF as no-ops.

**Resolution (implements “取消 AirCopy” as product feature, not FSK plumbing):**

1. Presets keep `ENABLE_AIRCOPY=false` and do **not** compile `ui/aircopy.c` / `DISPLAY_AIRCOPY`.
2. When `ENABLE_MESSENGER` is on, still compile `app/aircopy.c` (buffer + FSK helpers only).
3. In the copied `messenger_rf.c`, replace FSK-related `#ifdef ENABLE_AIRCOPY` with `#if defined(ENABLE_AIRCOPY) || defined(ENABLE_MESSENGER)` (or equivalent single helper macro).
4. BEAM stays fully off (`ENABLE_FEAT_F4HWN_BEAM=false`).
5. Game stays fully off (`ENABLE_FEAT_F4HWN_GAME=false`); `F+7` becomes HEARD.

CALLTX UI / `ACTION_OPT_CALLTX` / `CllTon` / `CllVol` menus are **not** ported. Keep `MSG_Config_t` fields for flash layout compatibility. Menu item `MENU_MSG_CALLTX` in GOGUFW is the **callsign TX toggle** (`callsign_tx`) — that **is** in scope (messaging setting), despite the enum name.

Mangosteen has no Survival Mode. Add `bool gSurvivalMode = false;` so copied GOGUFW sources compile; do not port Survival Mode activation.

---

## File map

### Create (copy from upstream pin)

| Path | Responsibility |
|---|---|
| `App/app/messenger.c` / `.h` | Open/tick/keys orchestration |
| `App/app/messenger_ui.c` / `.h` | Screens + `UI_DisplayMessenger` |
| `App/app/messenger_t9.c` / `.h` | T9 editor |
| `App/app/messenger_rf.c` / `.h` | FSK sidecar, ACK/retry (**patch AIRCOPY ifdef**) |
| `App/app/messenger_packet.c` / `.h` | `GGM2` codec + `MSG_PACKET_SelfTest` |
| `App/app/messenger_store.c` / `.h` | Config/drafts @ `0x012000` |

### Modify

| Path | Responsibility |
|---|---|
| `App/CMakeLists.txt` | `ENABLE_MESSENGER` sources; aircopy.c when messenger && !aircopy UI |
| `CMakePresets.json` | `ENABLE_MESSENGER=true`; force AirCopy/BEAM/Game false on all presets |
| `App/driver/eeprom_compat.c` | Map `0x012000` ↔ EEPROM `0x00E000` |
| `App/misc.h` / `App/misc.c` | `gSurvivalMode` stub |
| `App/ui/ui.h` / `App/ui/ui.c` | `DISPLAY_MESSENGER` + `UI_DisplayMessenger` |
| `App/settings.h` | `ACTION_OPT_MESSENGER`, `ACTION_OPT_HEARD` (no CALLTX) |
| `App/app/action.c` | Open Messenger / HEARD actions |
| `App/app/app.c` | Key table, IRQ, 10 ms ticks |
| `App/app/main.c` | `F+MENU` → Messenger; `F+7` → HEARD |
| `App/ui/menu.h` / `App/ui/menu.c` / `App/app/menu.c` | Msg* / RngRsp menus + shortcut labels |
| `App/ui/status.c` | Unread icon via `MSG_HasUnread()` |
| `App/main.c` or boot path | `MSG_Init` / store+RF open after EEPROM init |
| `README.md` | Short user-facing note |

---

### Task 1: Pin upstream and copy messenger modules

**Files:**
- Create: `App/app/messenger.c`, `messenger.h`, `messenger_ui.c`, `messenger_ui.h`, `messenger_t9.c`, `messenger_t9.h`, `messenger_rf.c`, `messenger_rf.h`, `messenger_packet.c`, `messenger_packet.h`, `messenger_store.c`, `messenger_store.h`

- [ ] **Step 1: Fetch pinned tree into a temp directory**

```powershell
$sha = "e334a5c6fff4bbecb328c49c009e4a58311593b0"
$tmp = Join-Path $env:TEMP "gogufw-messenger-$sha"
if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
git clone --depth 1 https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger.git $tmp
Set-Location $tmp
git fetch --depth 1 origin $sha
git checkout $sha
```

Expected: checkout succeeds at that SHA.

- [ ] **Step 2: Copy the twelve messenger files into Mangosteen**

```powershell
$dst = "D:\File\code\uvk1\mangosteen\App\app"
Copy-Item "$tmp\App\app\messenger*.c","$tmp\App\app\messenger*.h" $dst
Get-ChildItem $dst\messenger*
```

Expected: 12 files present under `App/app/`.

- [ ] **Step 3: Record provenance in the commit message later; do not edit UI strings yet**

- [ ] **Step 4: Commit (only if the user asked to commit in this session)**

```bash
git add App/app/messenger*.c App/app/messenger*.h
git commit -m "$(cat <<'EOF'
chore: vendor GOGUFW messenger sources at e334a5c

EOF
)"
```

---

### Task 2: Patch FSK ifdef + Survival stub so Messenger RF works without AirCopy UI

**Files:**
- Modify: `App/app/messenger_rf.c` (all FSK `#ifdef ENABLE_AIRCOPY` that wrap real TX/RX/sidecar — not comments)
- Modify: `App/misc.h`, `App/misc.c`

- [ ] **Step 1: Add Survival stub (always false)**

In `App/misc.h` (near other globals):

```c
/* Survival Mode not ported; keep symbol for GOGUFW messenger sources. */
extern bool gSurvivalMode;
```

In `App/misc.c`:

```c
bool gSurvivalMode = false;
```

- [ ] **Step 2: Introduce a local compile gate at the top of `messenger_rf.c`**

After includes:

```c
#if defined(ENABLE_AIRCOPY) || defined(ENABLE_MESSENGER)
#define MSG_RF_HAS_FSK_PATH 1
#else
#define MSG_RF_HAS_FSK_PATH 0
#endif
```

Replace FSK functional `#ifdef ENABLE_AIRCOPY` / `#endif` pairs that guard buffer/TX/RX/sidecar with `#if MSG_RF_HAS_FSK_PATH`. Keep any AirCopy-**UI**-only logic (if present) behind `ENABLE_AIRCOPY` alone.

- [ ] **Step 3: Grep to confirm no remaining hard `ENABLE_AIRCOPY` gates on send/receive**

```powershell
rg "ENABLE_AIRCOPY" App/app/messenger_rf.c
```

Expected: only comments, or `#if defined(ENABLE_AIRCOPY) || defined(ENABLE_MESSENGER)` / `MSG_RF_HAS_FSK_PATH`. Every `MSG_RF_SendText` / sidecar arm path must be active under `ENABLE_MESSENGER`.

- [ ] **Step 4: Commit (if user requested commits)**

```bash
git add App/app/messenger_rf.c App/misc.c App/misc.h
git commit -m "$(cat <<'EOF'
fix: enable messenger FSK path without AirCopy UI

EOF
)"
```

---

### Task 3: CMake + presets

**Files:**
- Modify: `App/CMakeLists.txt`
- Modify: `CMakePresets.json`

- [ ] **Step 1: Add `ENABLE_MESSENGER` feature block after flashlight (mirror GOGUFW)**

In `App/CMakeLists.txt` after `enable_feature(ENABLE_FLASHLIGHT ...)`:

```cmake
enable_feature(ENABLE_MESSENGER
    app/messenger.c
    app/messenger_store.c
    app/messenger_packet.c
    app/messenger_t9.c
    app/messenger_ui.c
    app/messenger_rf.c
)

# Messenger reuses aircopy.c FSK buffer helpers without AirCopy UI.
if(ENABLE_MESSENGER AND NOT ENABLE_AIRCOPY)
    target_sources(App INTERFACE
        app/aircopy.c
    )
endif()

if(ENABLE_MESSENGER OR ENABLE_AIRCOPY OR ENABLE_UART OR ENABLE_USB)
    # ensure crc + eeprom_compat available when messenger needs flash mapping helpers
endif()
```

Also change the existing:

```cmake
if(ENABLE_AIRCOPY OR ENABLE_UART OR ENABLE_USB)
    target_sources(App INTERFACE
        driver/crc.c
        driver/eeprom_compat.c
    )
endif()
```

to:

```cmake
if(ENABLE_AIRCOPY OR ENABLE_MESSENGER OR ENABLE_UART OR ENABLE_USB)
    target_sources(App INTERFACE
        driver/crc.c
        driver/eeprom_compat.c
    )
endif()
```

Avoid duplicating `crc.c` / `eeprom_compat.c` if both conditions would add them — keep a **single** `if` as above.

- [ ] **Step 2: Update `CMakePresets.json` `default` cacheVariables**

Add:

```json
"ENABLE_MESSENGER": true,
```

Ensure (already mostly true on default):

```json
"ENABLE_AIRCOPY": false,
"ENABLE_FEAT_F4HWN_BEAM": false,
"ENABLE_FEAT_F4HWN_GAME": false,
```

- [ ] **Step 3: Override child presets that re-enable AirCopy/Game/BEAM**

In `Bandscope`, `Broadcast`, `Basic`, `RescueOps`, `Game`, `Fusion` (and any other child that sets these), force:

```json
"ENABLE_MESSENGER": true,
"ENABLE_AIRCOPY": false,
"ENABLE_FEAT_F4HWN_BEAM": false,
"ENABLE_FEAT_F4HWN_GAME": false
```

Especially fix `Game` and `Fusion` which currently set Game/AirCopy/BEAM true.

- [ ] **Step 4: Configure + build Custom**

```bash
./compile-with-docker.sh Custom
```

Expected at this stage: may still fail on missing hooks (`DISPLAY_MESSENGER`, `ACTION_OPT_*`, etc.). If the failure is only undefined refs from missing glue, proceed to Task 4. If errors are inside vendor files (missing symbols like `gSurvivalMode`), fix before continuing.

---

### Task 4: Display enum + UI function table

**Files:**
- Modify: `App/ui/ui.h`, `App/ui/ui.c`

- [ ] **Step 1: Add display type in `ui.h`**

```c
#ifdef ENABLE_MESSENGER
    DISPLAY_MESSENGER,
#endif
```

Place before `DISPLAY_N_ELEM` (same order family as GOGUFW; keep Mangosteen `DISPLAY_RXTX_LOG` if present — put Messenger before `DISPLAY_N_ELEM` consistently with other optional screens).

- [ ] **Step 2: Wire display in `ui.c`**

```c
#ifdef ENABLE_MESSENGER
    #include "app/messenger_ui.h"
#endif

/* in UI_DisplayFunctions[] */
#ifdef ENABLE_MESSENGER
    [DISPLAY_MESSENGER] = &UI_DisplayMessenger,
#endif
```

Confirm `UI_DisplayMessenger` is declared in `messenger_ui.h` (from upstream).

- [ ] **Step 3: Build check**

```bash
./compile-with-docker.sh Custom
```

Expected: progress past `DISPLAY_N_ELEM` static_assert; may still fail on actions/app hooks.

---

### Task 5: Actions + settings enums (no CALLTX)

**Files:**
- Modify: `App/settings.h`
- Modify: `App/app/action.c`

- [ ] **Step 1: Extend `ACTION_OPT_t` in `settings.h`**

Before `ACTION_OPT_LEN`:

```c
#ifdef ENABLE_MESSENGER
    ACTION_OPT_MESSENGER,
    ACTION_OPT_HEARD,
#endif
```

Do **not** add `ACTION_OPT_CALLTX`.

- [ ] **Step 2: Add open helpers + table entries in `action.c`**

Mirror GOGUFW `ACTION_OpenMessenger` / `ACTION_OpenHeard` but omit Survival early-outs if desired (stub already false). Include:

```c
#ifdef ENABLE_MESSENGER
#include "app/messenger.h"
#endif
```

Table:

```c
#ifdef ENABLE_MESSENGER
    [ACTION_OPT_MESSENGER] = &ACTION_OpenMessenger,
    [ACTION_OPT_HEARD]     = &ACTION_OpenHeard,
#endif
```

Implement toggle-close behavior exactly as GOGUFW (`MSG_IsHomeOpen` / `MSG_RangeIsOpen`).

- [ ] **Step 3: Keep `static_assert(ARRAY_SIZE(action_opt_table) == ACTION_OPT_LEN);` green**

---

### Task 6: `app.c` IRQ, tick, key dispatch

**Files:**
- Modify: `App/app/app.c`

- [ ] **Step 1: Includes + ProcessKeys table**

```c
#ifdef ENABLE_MESSENGER
#include "app/messenger.h"
#include "app/messenger_rf.h"
#endif
```

In the display→key handler table:

```c
#ifdef ENABLE_MESSENGER
    [DISPLAY_MESSENGER] = &MSG_ProcessKeys,
#endif
```

- [ ] **Step 2: FSK interrupt path**

Where BK4819 interrupt status is handled (GOGUFW calls near existing FSK/aircopy handling), add:

```c
#ifdef ENABLE_MESSENGER
        if (!gSurvivalMode) MSG_RF_OnRadioInterrupt(interrupts.__raw);
#endif
```

Use the same `interrupts` variable name already in Mangosteen `app.c` (adjust if the local name differs).

- [ ] **Step 3: 10 ms tick**

In the 10 ms timeslice (alongside other periodic work):

```c
#ifdef ENABLE_MESSENGER
        if (!gSurvivalMode) {
            MSG_RF_Tick10ms();
            MSG_Tick();
        }
#endif
```

- [ ] **Step 4: Call `MSG_RF_OnRadioSetupRegisters` from Mangosteen’s `RADIO_SetupRegisters` hook if GOGUFW does** — search upstream `messenger_rf` / `radio.c` for `MSG_RF_OnRadioSetupRegisters` and mirror that single call site in Mangosteen `radio.c` or `app.c`.

---

### Task 7: `main.c` shortcuts `F+MENU` / `F+7`

**Files:**
- Modify: `App/app/main.c`

- [ ] **Step 1: Replace Game on `KEY_7` with HEARD**

Current Mangosteen block uses `ENABLE_FEAT_F4HWN_GAME` + `APP_RunBreakout()`. Replace with GOGUFW pattern:

```c
        case KEY_7:
#ifdef ENABLE_MESSENGER
            if (!beep) {
                if (!gSurvivalMode) MSG_RangeOpen();
                else gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            } else
#endif
            {
#ifdef ENABLE_VOX
                ACTION_Vox();
#endif
            }
            break;
```

Remove `#include "app/breakout.h"` usage under game ifdef if unused.

- [ ] **Step 2: `F+MENU` opens Messenger**

In `MAIN_Key_MENU`, before normal menu open, add GOGUFW block:

```c
#ifdef ENABLE_MESSENGER
    if (!bKeyHeld && !bKeyPressed && gWasFKeyPressed) {
        gWasFKeyPressed = false;
        if (!gSurvivalMode) {
            MSG_Open();
            gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
        } else {
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        }
        return;
    }
#endif
```

Confirm Mangosteen already has `gWasFKeyPressed` (it should, as F-key chord state).

- [ ] **Step 3: Include `app/messenger.h` under `ENABLE_MESSENGER`**

---

### Task 8: Flash mapping `0x012000`

**Files:**
- Modify: `App/driver/eeprom_compat.c`

- [ ] **Step 1: Append GOGUFW messenger mapping after Boot Logo**

```c
    _MK_MAPPING(0x012000, 0x00E000, 0x00F000),  // Messenger/call settings sector (4 KB)
                                                // CHIRP-visible alias only. Actual storage remains 0x012000.
```

Do **not** add `0x013000` FM Naming mapping.

- [ ] **Step 2: Confirm `messenger_store.c` still uses `MSG_CFG_FLASH_ADDR 0x012000u` and `PY25Q16_*` directly (no edit required if unchanged from upstream).**

---

### Task 9: Menus (messaging only)

**Files:**
- Modify: `App/ui/menu.h`, `App/ui/menu.c`, `App/app/menu.c` (acceptance/save paths as in GOGUFW)

- [ ] **Step 1: Add menu IDs in `menu.h` enum (before end of enum)**

Port from GOGUFW under `#ifdef ENABLE_MESSENGER`:

- `MENU_MSG_RX`
- `MENU_MSG_CSG` (callsign edit)
- `MENU_MSG_CALLTX` (**callsign_tx toggle** — keep name for parity)
- `MENU_MSG_ACK`
- `MENU_MSG_BEEP`
- `MENU_MSG_LED`
- `MENU_MSG_HOP`
- `MENU_RNG_RSP`
- `MENU_MSG_DEBUG` (hidden)

Do **not** add `MENU_CALL_TONE` / `MENU_CALL_VOL`.

- [ ] **Step 2: Add `MenuList[]` entries in `ui/menu.c`**

```c
#ifdef ENABLE_MESSENGER
    {"MsgRx",       MENU_MSG_RX       },
    {"MsgCsg",      MENU_MSG_CSG      },
    {"MsgAck",      MENU_MSG_ACK      },
    {"MsgBep",      MENU_MSG_BEEP     },
    {"MsgLed",      MENU_MSG_LED      },
    {"RngRsp",      MENU_RNG_RSP      },
#ifdef ENABLE_MESSENGER /* debug hidden */
    {"MsgDbg",      MENU_MSG_DEBUG    },
    {"MsgHop",      MENU_MSG_HOP      },
#endif
#endif
```

Match GOGUFW ordering/visibility (`FIRST_HIDDEN_MENU_ITEM = MENU_MSG_DEBUG` when messenger enabled — merge carefully with Mangosteen’s existing hidden-menu logic).

- [ ] **Step 3: Shortcut submenu labels**

In the action-name table used by F1/F2/MENU long shortcuts, add:

```c
#ifdef ENABLE_MESSENGER
    {"MESSENGER",       ACTION_OPT_MESSENGER},
    {"HEARD",           ACTION_OPT_HEARD},
#endif
```

No `CALLTX` label.

- [ ] **Step 4: Port get/set/accept handlers from GOGUFW `ui/menu.c` + `app/menu.c` for the Msg* / RngRsp items only**

Copy the `case MENU_MSG_*` / `MENU_RNG_RSP` bodies that read/write `gMessengerConfig` and call `MSG_STORE_SaveConfig()`. Skip `CllTon` / `CllVol` cases.

---

### Task 10: Status unread + boot init

**Files:**
- Modify: `App/ui/status.c`
- Modify: `App/main.c` (or `App/helper/boot.c` if that is where EEPROM init completes)

- [ ] **Step 1: Unread icon**

In GOGUFW `status.c`, find `MSG_HasUnread()` usage and port the same `#ifdef ENABLE_MESSENGER` paint/bit. Include `app/messenger.h` or `messenger_store.h` as upstream does.

- [ ] **Step 2: Init after settings load**

After `SETTINGS_InitEEPROM()` (or equivalent) in `App/main.c`:

```c
#ifdef ENABLE_MESSENGER
    MSG_Init();   /* if provided; else MSG_STORE_Init(); MSG_RF_Open(); */
#endif
```

Verify against upstream `messenger.c` for the exact public init API (`MSG_Init` vs store+RF open). Call whatever upstream boot path uses.

- [ ] **Step 3: Optional debug self-test (hostless firmware check)**

Under `#if defined(ENABLE_MESSENGER) && defined(ENABLE_FEAT_F4HWN_DEBUG)` once at boot:

```c
    if (!MSG_PACKET_SelfTest()) {
        // leave a breakpoint-friendly side effect; do not brick boot in release
    }
```

Only if `MSG_PACKET_SelfTest` exists in the vendored `messenger_packet.c`.

---

### Task 11: Full Custom build green

**Files:**
- Any remaining compile fixes only (missing includes, enum order, `#ifdef` mismatches)

- [ ] **Step 1: Build**

```bash
./compile-with-docker.sh Custom
```

Expected: success; firmware artifact under `build/Custom` (or project’s usual output path).

- [ ] **Step 2: Confirm feature flags in build log / `cmake` cache**

```bash
grep -E "ENABLE_MESSENGER|ENABLE_AIRCOPY|ENABLE_FEAT_F4HWN_BEAM|ENABLE_FEAT_F4HWN_GAME" build/Custom/CMakeCache.txt
```

Expected:

```
ENABLE_MESSENGER:BOOL=ON
ENABLE_AIRCOPY:BOOL=OFF
ENABLE_FEAT_F4HWN_BEAM:BOOL=OFF
ENABLE_FEAT_F4HWN_GAME:BOOL=OFF
```

- [ ] **Step 3: Build Fusion preset too (historically enabled Game/BEAM/AirCopy)**

```bash
./compile-with-docker.sh Fusion
```

Expected: success with the same three features forced off and Messenger on.

---

### Task 12: README note

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add a short section**

```markdown
## Messenger

Mangosteen includes GOGUFW-compatible FSK Messenger (Inbox / Compose / Sent / Drafts / ACK), HEARD (`F+7`), and Range Check.

- Interop: same `GGM2` packets as [GOGUFW-UV-K1-Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger)
- Shortcuts: `F+MENU` Messenger, `F+7` HEARD
- Storage: private flash sector `0x012000`
- Disabled in favor of Messenger FSK: AirCopy UI, BEAM, Breakout game
```

- [ ] **Step 2: Commit all remaining work if the user requested a commit**

---

### Task 13: Device verification checklist (manual)

No automated radio harness in-repo. After flashing Custom firmware:

- [ ] **Step 1: UI smoke** — `F+MENU` opens Messenger; compose with T9; navigate Inbox/Sent/Drafts; exit to home cards.
- [ ] **Step 2: HEARD** — `F+7` opens HEARD/Range UI (not Breakout).
- [ ] **Step 3: Interop** — exchange TEXT with a GOGUFW radio on the same frequency/mode assumptions as GOGUFW docs; enable MsgAck and confirm Sent ACK source.
- [ ] **Step 4: Sidecar RX** — stay on home cards, receive a message, confirm unread status indication + optional beep.
- [ ] **Step 5: Range Check** — run Ping/Pong; list updates.
- [ ] **Step 6: Flash** — set callsign/draft, power cycle, values persist; logo/channels intact.
- [ ] **Step 7: Regression** — home cards, WFM/FM path, scan still work; no AirCopy/BEAM/Game entry points.

---

## Self-review vs spec

| Spec requirement | Task |
|---|---|
| Port Messenger + HEARD + Range Check + drafts + ACK | 1, 5–7, 9–10 |
| GOGUFW interop / GGM2 unchanged | 1 (vendor packet), no protocol edits |
| UI as-is | 1 (vendor UI) |
| Flash `0x012000` + eeprom map | 8 |
| Disable BEAM / AirCopy UI / Game | 2–3, 7 |
| `F+7` HEARD | 7 |
| No CALLTX / Survival / FM Naming / CHIRP | 5, 9 (explicit skips) |
| Home cards unchanged | no `ui/main.c` home redesign |
| Testing | 11 (build), 13 (device) |
| FSK works with AirCopy UI off | 2 + CMake aircopy.c exception |

**Placeholder scan:** none intentional.  
**Type consistency:** `ENABLE_MESSENGER`, `ACTION_OPT_MESSENGER` / `ACTION_OPT_HEARD`, `DISPLAY_MESSENGER`, `MSG_*` APIs from vendored headers.
