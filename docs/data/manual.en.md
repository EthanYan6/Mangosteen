# Mangosteen User Manual

This manual has two chapters:

1. **Chapter 1 — Flashing Guide**: flash firmware, font, calibration, and related tools in the browser  
2. **Chapter 2 — System User Guide**: using the radio, scan, save channels, naming/IME, and SMS after Mangosteen is installed

> Compatible with Quansheng **UV-K1 / UV-K5 / UV-K6 V3 only**. Check the rear label for **V3**.

---

# Chapter 1 Flashing Guide

Welcome to the **Mangosteen Web Flasher**. This chapter covers backup calibration, firmware flashing, font flashing, calib check/restore, frequency programming, and boot logo.

## 1.1 Before You Start

1. Use **Chrome** or **Edge** (Web Serial required).
2. Use a genuine programming cable and connect the radio to the PC.
3. Before the first third-party flash, **backup calibration once** on stock firmware in the normal UI.

## 1.2 Recommended Order

Especially for the first Mangosteen install:

1. **Backup calibration** (stock firmware, normal UI)
2. **Flash firmware** (BOOT mode)
3. **Flash font** (normal UI; needed for Chinese UI / channel names)
4. **Check calibration** (optional)
5. **Restore calibration** (normal UI, using your backup)
6. **Freq programming / boot logo** (as needed)

## 1.3 Backup Calibration

**Requirement:** Radio in the normal UI (channels/menus visible). Do **not** stay on the Bootloader-only screen.

1. Open the site → **Backup Calib**.
2. Click **Export calibration**, pick the serial port, and connect.
3. Download `calibration.dat` and keep it safe.

Tip: Backup once before leaving stock firmware. Prefer that original file for later restore.

## 1.4 Flash Firmware

**Requirement:** **BOOT mode**.

How: Power off, **hold PTT**, then turn the knob to power on.

1. Open **Flash Firmware**.
2. **Remote Fetch** the latest `mangosteen_*.bin`, or **Local Select** a file.
3. Confirm V3 hardware, then flash and wait.
4. Do not unplug, close the tab, or cut power during flashing.

## 1.5 Flash Font

**Requirement:** Normal UI (**not** BOOT).

1. Open **Flash Font**.
2. **Remote Fetch** the site default font pack, or choose a local `.bin`.
3. Click **Flash font** and wait.

Layout: 16×16 at `0xA0000`, 8×8 at `0xE0000`. Without the font, Chinese menus, names, and SMS may look wrong.

## 1.6 Check Calibration

1. Connect USB in the normal UI.
2. Open **Check Calib** to read device data, load a backup, and compare.
3. Write to the intended address only after the values look right.

Mangosteen calibration address is fixed at `0xB000`.

## 1.7 Restore Calibration

**Requirement:** Normal UI.

1. Open **Restore Calib**.
2. Select your `.dat` backup (512 bytes, format from this site).
3. Restore; the radio usually reboots when done.

## 1.8 Frequency Programming

**Requirement:** Normal UI.

1. Open **Freq Program**.
2. Read from the device, edit the table, then write back.
3. Excel import/export is supported. Names support ASCII/Chinese (GB2312, up to ~10 bytes).
4. Disconnect after read/write is normal.

Targets **Mangosteen** firmware only. Modes include FM / AM / USB / WFM.

## 1.9 Boot Logo

**Requirement:** Normal UI (not BOOT). Follow the **Boot Logo** tab, then reboot to preview.

## 1.10 Flashing FAQ

### No serial port / device not found

Use Chrome or Edge and confirm the cable driver / COM port.

### Firmware flash connection fails

Confirm BOOT (hold PTT while powering on). Try another cable or USB port.

### Snow / garbled screen after flash

Usually calibration. Restore the original `calibration.dat` in the normal UI.

### Missing or broken Chinese glyphs

Flash the site font pack.

### Remote fetch failed

Download the Release binary manually, then use **Local Select**.

## 1.11 Safety Notes

- Confirm **V3** hardware.
- Keep power and USB stable during writes.
- Back up calibration and channels first.
- This site uses Web Serial in your browser and does not upload your calib/channel files.

---

# Chapter 2 System User Guide

This chapter covers daily use after Mangosteen is installed. On this keypad, **`#` is the F key** (short press arms F-combos; long press locks the keypad). **`*`** is often used for scan and IME mode switching.

## 2.1 Using the Radio

### Power and volume

- **Power / volume:** physical volume knob. Holding **PTT** while powering on enters BOOT (flashing only).
- Menu **SetVol** adjusts speaker gain; everyday listening is still mainly the knob.

### Main menu

1. Short **MENU** opens the menu.
2. **↑ / ↓** browse items; digits can jump by menu number.
3. **MENU** enters a submenu; **EXIT** backs out.
4. Destructive actions show **SURE?** — press **MENU** again to confirm.

Switch UI language via **Lang** (English / Chinese).

### VFO vs MR (memory channels)

| Action | Keys |
|--------|------|
| Toggle VFO ↔ MR | **F + 3** (short `#`, then `3`) |
| Copy MR → VFO | **F + 1** (in channel mode) |
| Band change (VFO) | **F + 1** (in freq mode) |

- **VFO:** ↑/↓ steps frequency; digits enter a frequency.
- **MR:** ↑/↓ changes channel; digits jump to a channel number.

### A / B and RxMode

| Action | Keys |
|--------|------|
| Switch A ↔ B | **F + 2** |

If **RxMode** is **MAIN ONLY**, **F+2** is refused with a beep — there is no second VFO to show.

| Mode | Meaning |
|------|---------|
| MAIN ONLY | Main VFO only; A/B switch disabled |
| DUAL RX RESPOND | Watch both; RX can bring that side to front |
| CROSS BAND | Cross-band style TX/RX |
| MAIN TX DUAL RX | Always TX on main; RX both |

### PTT, power, modulation

- **PTT:** side PTT key.
- **TX power:** try **F + 6**, a side **POWER** action, or menu **Power**.
- **Modulation** (FM / AM / USB / WFM…): side **MODE** or menu **Mode**.

### Useful F-keys (home screen)

| Combo | Function |
|-------|----------|
| F + 2 | A/B |
| F + 3 | VFO / MR |
| F + 4 | Freq/CSS scanner screen |
| F + 5 | Scan-list / scan-range helpers |
| F + 6 | TX power |
| F + 7 | HEARD / range check |
| F + MENU | Open Messenger |
| Long * | Start / stop channel or frequency scan |
| F + * | CTCSS/DCS code search |

Side keys (**F1 / F2 / long M**, etc.) can be assigned to SCAN, MONITOR, MESSENGER, HEARD, MAIN ONLY, and more.

---

## 2.2 Scanning

### Start / stop

1. On the home screen, **long-press `*`**, or press a side key bound to **SCAN**.
2. While scanning, short **EXIT** or **MENU** stops.
3. **↑ / ↓** change scan direction.
4. In MR mode, pressing SCAN again while already scanning cycles scan lists.

### Scan types

- **Channel (MR) scan:** walks channels in the active scan list(s).
- **Frequency (VFO) scan:** steps by the current VFO step.
- **Scan range:** in VFO, **F+5** sets a range related to the other VFO frequency; long `*` scans inside it; F+5 again clears the range.
- **CTCSS/DCS search:** **F + `*`**.
- **F+4:** dedicated freq/CSS scanner UI.

### Skip / exclude

- While paused on a hit, **long MENU** excludes that channel and continues.
- **F+5** adjusts that channel’s scan-list membership.

### Related menus

| Menu | Role |
|------|------|
| ScList | Default list / ALL |
| ScPri / PriCh | Priority scan / channels |
| ScnRev | Resume behavior after a hit |
| ChList | Channel list membership |

---

## 2.3 Saving Channels

Store the current VFO settings into a memory channel:

1. Tune frequency, CTCSS/DCS, power, modulation, etc. on the home screen.
2. **MENU** → **ChSave** → **MENU**.
3. Choose the destination with **↑/↓** or a 4-digit channel number.
4. **MENU** → **SURE?** → **MENU** again to save.

Delete via **ChDele** (same confirm pattern).

You can also bulk-import channels from the website **Freq Program** tab.

---

## 2.4 Channel Naming and Input Method

### Enter naming

1. **MENU** → **ChName**.
2. Select a valid channel → **MENU** to edit.
3. Short **MENU** advances the cursor; at the end, confirm **SURE?** to save.
4. Names are up to about **10 bytes** (ASCII characters, or GB2312 pairs ≈ 5 Chinese characters).

> Chinese display needs the font flash.

### Input modes (ChName only)

Short **`#` (F)** cycles:

| Badge | Mode |
|-------|------|
| `abc` | Lowercase multitap |
| `ABC` | Uppercase multitap |
| `123` | Digits only |
| `pinyin` | T9 pinyin → Chinese characters |

Notes:

- Outside pinyin, long `#` inserts `#`.
- In pinyin, `*` is backspace; in other modes `*` can insert `-`, etc.
- Long-press a digit to force that digit.

### Pinyin tips

1. Short `#` until `pinyin`.
2. Enter T9 digits with **2–9** using the letter map below (e.g. `ni` → `64`, `hao` → `426`).
3. **1** enters syllable/candidate confirm; **↑/↓** page or move candidates.
4. Pick by number; **MENU** confirms depending on compose state.
5. Backspace with **`*`**; **0** inserts a space.

#### Pinyin keypad letter map

In pinyin mode, letters follow the standard phone T9 layout:

| Key | Letters |
|-----|---------|
| **1** | Confirm / pick (then press a number to choose a character) |
| **2** | A B C |
| **3** | D E F |
| **4** | G H I |
| **5** | J K L |
| **6** | M N O |
| **7** | P Q R S |
| **8** | T U V |
| **9** | W X Y Z |
| **0** | Space |
| **\*** | Backspace |
| **↑ / ↓** | Next syllable / candidate page |

Example: for “山竹”, type `shan` = `7426`, `zhu` = `948`, then pick from candidates.

Callsign / MsgCsg / DTMF editors generally **do not** support pinyin — ASCII/digit style only.

---

## 2.5 Using SMS (Messenger)

Mangosteen includes Messenger (title **MESSENGER** / **信息**), interoperable with compatible GOGUFW-style messaging. Messages can be received in the background; an envelope may appear on the status line when unread.

### Open / exit

| Action | Keys |
|--------|------|
| Open Messenger | **F + MENU** ( `#` then MENU), or side **MESSENGER** |
| Open HEARD / Range | **F + 7** or side **HEARD** |
| Exit to home | **EXIT** from Messenger home |

### Home items

↑/↓ to select, **MENU** to enter:

| EN | CN | Role |
|----|----|------|
| INBOX | 收件箱 | Received messages |
| COMPOSE | 编辑信息 | Write a new message |
| SENT | 发件箱 | Sent messages |
| DRAFTS | 草稿箱 | Drafts |

### Compose and send

1. Open **COMPOSE**.
2. Enter text (about **36** bytes max).
3. **MENU** sends (empty input sends an empty-message marker).
4. **EXIT** cancels without sending.

From an inbox read, **MENU** replies with a `RE: ` prefix. From sent, **MENU** can resend. **`#` (F)** deletes in list/read views (UI shows a delete hint).

Notes:

- **Inbox / Sent are RAM-only** and may be cleared on power-off.
- **Drafts and callsign** persist in Flash.
- Set **MsgCsg** (local name/callsign) in the radio menu so others can identify you.

### Compose IME

Short **`*`** cycles:

| Badge | Mode |
|-------|------|
| `B` | Uppercase multitap |
| `b` | Lowercase multitap |
| `2` | Numeric |
| `pinyin` | Same pinyin engine as ChName (see **2.4 Pinyin keypad letter map**) |

- Backspace: **`#` (F)**.
- In letter modes, long-press a digit to insert that digit.
- `0` is usually space; `1` is punctuation in letter modes.

### Messenger radio menu

| Menu | Role |
|------|------|
| MsgRx | Enable message receive |
| MsgCsg | Edit local callsign/name (~6 chars) |
| MsgAck | Auto-ack |
| MsgBep | Beep on receive |
| MsgLed | Light on receive |
| RngRsp | Range-check response |

### HEARD / Range Check

**F+7** opens it. **MENU** starts/stops ranging; ↑/↓ scroll heard stations (callsign, RSSI, age); **EXIT** leaves.

---

## 2.6 Usage Notes

1. **MAIN ONLY** disables A/B switching.
2. Chinese display needs the font — flash it as described in Chapter 1.
3. While scanning, dual-watch UI may temporarily behave as single-VFO scan until you stop.
4. Inbox content may be lost on power-off; set your local name before messaging.
