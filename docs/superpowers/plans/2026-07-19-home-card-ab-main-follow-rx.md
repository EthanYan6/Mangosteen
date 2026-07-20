# Home Card A/B + Main Follow RX Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show A/B left of the home-card channel number; when dual RX is on, promote the receiving VFO to main/TX/front and keep it until the other VFO RX or long-press 2.

**Architecture:** Front card already draws `TX_VFO`. Add A/B in `HC_DrawFront`. In `APP_StartListening` (DW hold block), if `TX_VFO != RX_VFO`, set main to `RX_VFO` and retarget DW/XB bookkeeping without `gFlagReconfigureVfos` (avoids mid-RX `RADIO_SetupRegisters` killing audio).

**Tech Stack:** C firmware (uv-k5 / F4HWN), ST7565 home card UI.

**Spec:** `docs/superpowers/specs/2026-07-19-home-card-ab-main-follow-rx-design.md`

---

### Task 1: Draw A/B left of channel number

**Files:**
- Modify: `App/ui/home_card.c` (`HC_DrawFront`)

- [x] **Step 1:** Before drawing the channel label, draw `"A"` or `"B"` via `HC_Small` at a fixed x left of the existing channel x (68), e.g. x=60–62.
- [x] **Step 2:** Keep battery at x=82; only nudge channel x if A/B collides.

---

### Task 2: Align main VFO to receiving VFO on dual-RX listen

**Files:**
- Modify: `App/app/app.c` (`APP_StartListening`, dual-watch block ~742–766)

- [x] **Step 1:** Inside existing `SCAN_OFF && DUAL_WATCH != OFF` block, when `TX_VFO != RX_VFO`:
  - `TX_VFO = RX_VFO`
  - retarget `CROSS_BAND_RX_TX` / `DUAL_WATCH` to `TX_VFO + 1` if enabled (same as `COMMON_SwitchVFOs`)
  - `gTxVfo = &gEeprom.VfoInfo[TX_VFO]` (no mid-RX `RADIO_SelectCurrentVfo` — it is static in `radio.c`)
  - `gRequestSaveSettings = 1`; `gUpdateDisplay = true`
- [x] **Step 2:** Do **not** set `gFlagReconfigureVfos` here (would call `RADIO_SetupRegisters` later and disrupt RX audio).

---

### Task 3: Smoke-check build (if toolchain available)

- [ ] **Step 1:** Configure/build the usual firmware preset; fix compile errors only.
- [ ] **Step 2:** Manual device checks per spec acceptance list (no automated UI tests in-repo).
