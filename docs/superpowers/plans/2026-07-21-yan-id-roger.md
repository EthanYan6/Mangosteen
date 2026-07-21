# Yan ID Roger Implementation Plan

> **For agentic workers:** Implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Yan ID setting and Roger=`YAN ID` end-of-TX FSK burst so peers show `CALL SIGN:` on the home DTMF line.

**Architecture:** Persist `yan_id` in EEPROM gap; extend Roger enum; GGM2 type 5 packet over existing Messenger FSK; dedicated RX display buffer with priority over DTMF.

**Tech Stack:** C firmware, BK4819 FSK, PY25Q16 settings, GGM2 wire format.

**Spec:** `docs/superpowers/specs/2026-07-21-yan-id-roger-design.md`

---

### Task 1: Settings + Roger enum

**Files:**
- Modify: `App/settings.h`, `App/settings.c`

- [ ] Add `ROGER_MODE_YAN_ID` and `char yan_id[7]` to `gEeprom`
- [ ] Load/save Yan ID at `0x00A0A8+0x20`; Roger max becomes 4

### Task 2: Menus

**Files:**
- Modify: `App/ui/menu.h`, `App/ui/menu.c`, `App/app/menu.c`

- [ ] Add `MENU_YAN_ID` ("Yan ID") text edit (max 6, alnum)
- [ ] Extend `gSubMenu_ROGER` with `"YAN ID"`

### Task 3: Packet + RF

**Files:**
- Modify: `App/app/messenger_packet.h/.c`, `App/app/messenger_rf.h/.c`

- [ ] `MSG_PKT_TYPE_YAN_ID = 5`, `MSG_PACKET_BuildYanId`
- [ ] `MSG_RF_SendYanId()`; RX path handles type 5 → display only (no SMS/HEARD)

### Task 4: Roger TX + UI display

**Files:**
- Modify: `App/driver/bk4819.c`, `App/ui/home_card.c`, `App/ui/main.c`, display globals

- [ ] `BK4819_PlayRoger`: Yan ID branch
- [ ] Priority `CALL SIGN:` on home/classic DTMF lines

### Task 5: Verify build

- [ ] Configure/build firmware preset; fix compile errors
