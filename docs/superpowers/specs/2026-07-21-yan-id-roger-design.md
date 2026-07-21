# Yan ID Roger Tail Design

**Date:** 2026-07-21  
**Status:** Approved for implementation  
**Approach:** Reuse GGM2 FSK modem; dedicated packet type; settings separate from Messenger callsign

## Goal

Let two Mangosteen radios exchange a short alphanumeric callsign at end-of-TX (roger position). The peer home DTMF line shows `CALL SIGN:` plus the decoded ID.

## Menu & settings

| Item | Behavior |
|------|----------|
| **Yan ID** | Text edit, letters + digits, max 6 chars. Stored separately from Messenger `callsign`. |
| **Roger** | Existing `OFF` / `ROGER` / `MDC`, plus new **`YAN ID`**. |

No separate “Enable Tx ID” item. Transmit Yan ID only when:

1. `gEeprom.ROGER == ROGER_MODE_YAN_ID`, and  
2. Yan ID is non-empty after trim.

If Roger is `YAN ID` but Yan ID is empty → behave like `OFF` (no roger beep, no FSK).

### Storage

- `ROGER_MODE_YAN_ID` extends `ROGER_Mode_t` (value 3). Same EEPROM byte as today (`0x00A0A8+0x18` Data[1]); load accepts `0..3`.
- `gEeprom.yan_id[7]` (6 chars + NUL) persisted in the unused gap at `0x00A0A8+0x20` (8 bytes). Independent of `gMessengerConfig.callsign`.

## Air protocol

Reuse BK4819 AirCopy/Messenger FSK path and GGM2 wire layout (`MSG_PKT_WIRE_LEN` 94), but:

- New type: `MSG_PKT_TYPE_YAN_ID = 5`
- `from` = Yan ID (padded/truncated to callsign field)
- `to` = `ALL`
- payload empty / unused
- CRC16 unchanged

### Isolation from Messenger SMS

- TEXT / ACK / PING / PONG parsers **must not** accept type 5.
- Type 5 handler **must not** write Inbox, Outbox, HEARD, or trigger range/ACK logic.
- TX uses `ignore_self_rx` so the sender does not locally display its own ID.

## TX timing

Hook in `BK4819_PlayRoger()` (called from `RADIO_SendEndOfTransmission`):

- `ROGER` → existing beep  
- `MDC` → existing MDC  
- `YAN ID` + non-empty ID → `MSG_RF_SendYanId()` (one FSK frame via existing packet send helper)  
- else → nothing  

Then existing DTMF end-of-TX and CSS tail continue as today.

## RX display

On valid type-5 frame with non-empty `from`:

- Copy up to 6 chars into a dedicated RX buffer (not `gDTMF_RX_live`).
- Set a timeout (same order of magnitude as live DTMF timeout).
- Home card DTMF line (`HC_FillDtmf`) and classic main DTMF line: **prefer** `CALL SIGN:<id>` over live DTMF / other DTMF status when the Yan ID buffer is active.
- Clear on timeout; do not show on the transmitter.

## Error handling

- FSK busy / send failure: skip silently; do not block PTT release.
- Bad CRC / wrong type: ignore.
- Non-Mangosteen peers: hear a brief digital burst only; no UI change.

## Non-goals

- No start-of-PTT ID burst.
- No merging with Messenger callsign / `callsign_tx`.
- No DTMF encoding of alphanumeric IDs.
