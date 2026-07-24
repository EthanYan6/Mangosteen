# Home Card Gauge Height −10%

Date: 2026-07-24  
Status: Approved (approach B)

## Goal

Reduce the home-card S-meter gauge vertical radius so the arc looks less tall / oval. Layout of frequency, channel name, status, and bottom bar stays unchanged.

## Decisions (locked)

| Topic | Decision |
|-------|----------|
| Approach | B — flatten vertical radius; rescale tick/needle Y |
| Horizontal radius | Keep `GAUGE_RX = 20` |
| Vertical radius | `GAUGE_RY` 18 → 16 (~−11%; nearest integer to −10%) |
| Center | Keep `GAUGE_CX = 25`, `GAUGE_CY = 24` |
| Thickness | Keep `GAUGE_THICK = 2` |
| Tick X | Keep `hc_tick_ox[]` / `hc_tick_ix[]` |
| Tick Y | Scale `hc_tick_oy[]` / `hc_tick_iy[]` by 16/18, round to nearest int |
| Other UI | No layout moves for name, frequency, status, bottom bar, card frames |

## Tick Y values

Scale factor `16/18`. Round half away from zero / nearest:

| Index | oy old → new | iy old → new |
|-------|--------------|--------------|
| 0 | 14 → 12 | 11 → 10 |
| 1 | 12 → 11 | 9 → 8 |
| 2 | 5 → 4 | 4 → 4 |
| 3 | −3 → −3 | −2 → −2 |
| 4 | −10 → −9 | −8 → −7 |
| 5 | −14 → −12 | −11 → −10 |
| 6 | −13 → −12 | −10 → −9 |
| 7 | −8 → −7 | −6 → −5 |
| 8 | 0 → 0 | 0 → 0 |

Resulting arrays:

```c
static const int8_t hc_tick_oy[9] = { 12, 11,  4, -3, -9,-12,-12, -7,  0};
static const int8_t hc_tick_iy[9] = { 10,  8,  4, -2, -7,-10, -9, -5,  0};
```

Update the comment above the tick tables to state `GAUGE_RX=20, GAUGE_RY=16`.

## Implementation touchpoints

| Area | File | Change |
|------|------|--------|
| Gauge radius | `App/ui/home_card.c` | `#define GAUGE_RY 16` |
| Tick / needle tips | `App/ui/home_card.c` | Replace `hc_tick_oy` / `hc_tick_iy`; comment |

No header or call-site changes. Arc, soft-fill, and needle already use `GAUGE_RY` / tick tables.

## Acceptance

- [ ] On device (or simulator), gauge arc is visibly flatter; less “tall ellipse”
- [ ] Ticks and needle tip stay inside the inner rim (no poking past outer arc)
- [ ] Frequency, channel name, status line, bottom bar, card stack offsets unchanged
- [ ] Build succeeds with existing preset
