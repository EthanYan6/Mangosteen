# Home Card A/B Swap Animation Implementation Plan

> **For agentic workers:** Implement task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Animate manual A/B card swap on the home card; disable A/B switch in MAIN ONLY with a short beep.

**Architecture:** Gate and request animation inside `COMMON_SwitchVFOs`. Home card keeps outgoing/incoming VFO + progress, draws both cards with pixel offsets, advances on 10ms ticks.

**Tech Stack:** C firmware (`ENABLE_FEAT_F4HWN` home card), ST7565 framebuffer, existing 10ms time slice.

---

### File map

| File | Responsibility |
|------|----------------|
| `App/app/common.c` / `.h` | MAIN ONLY reject + beep; start anim after switch |
| `App/ui/home_card.c` / `.h` | Anim state, offset draw, 10ms / cancel APIs |
| `App/app/app.c` | Pump 10ms anim → redraw |

---

### Task 1: APIs + MAIN ONLY gate

- [x] Add `UI_HomeCard_StartVfoSwapAnim(uint8_t outgoing_vfo)`, `UI_HomeCard_CancelVfoSwapAnim`, `UI_HomeCard_TimeSlice10ms` to `home_card.h`
- [x] In `COMMON_SwitchVFOs`: if MAIN ONLY → beep and return; else switch then start anim (under `ENABLE_FEAT_F4HWN`)

### Task 2: Offset draw + animation frames

- [x] Add `hc_ox`/`hc_oy` applied in `HC_Pixel` and `HC_Small`
- [x] During anim, `UI_DisplayHomeCard` draws incoming then outgoing with interpolated offsets
- [x] Skip or small-font the bold name when offset ≠ 0

### Task 3: 10ms pump + cancel

- [x] `APP_TimeSlice10ms`: if home-card anim active, advance and set `gUpdateDisplay`
- [x] Cancel anim when leaving main display or when interrupted (snap to normal draw)

### Task 4: Verify

- [x] Build firmware preset if available; fix compile errors
