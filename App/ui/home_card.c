/* Copyright 2026
 *
 * Card-style home screen for single and dual watch.
 */

#include "ui/home_card.h"

#ifdef ENABLE_FEAT_F4HWN

#include <string.h>

#include "app/dtmf.h"
#include "bitmaps.h"
#include "dcs.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/battery.h"
#include "ui/helper.h"
#include "ui/home_card_font.h"
#include "ui/main.h"
#include "ui/ui.h"

#ifdef ENABLE_AM_FIX
#include "am_fix.h"
#endif

/* Screen coords: y 0..7 = status line, y 8..63 = frame buffer. */
#define HC_W            LCD_WIDTH
#define HC_H            LCD_HEIGHT

#define FRONT_X0        2
#define FRONT_Y0        1
#define FRONT_X1        119
#define FRONT_Y1        54

#define BACK_OX         6
#define BACK_OY         8

#define GAUGE_CX        28
#define GAUGE_CY        24
#define GAUGE_R         18
#define GAUGE_THICK     2

static void HC_Pixel(uint8_t x, uint8_t y, bool on)
{
	if (x >= HC_W || y >= HC_H)
		return;
	if (y < 8)
		PutPixelStatus(x, y, on);
	else
		PutPixel(x, y - 8, on);
}

static void HC_HLine(uint8_t x0, uint8_t x1, uint8_t y, bool on)
{
	if (x0 > x1) {
		uint8_t t = x0;
		x0 = x1;
		x1 = t;
	}
	for (uint8_t x = x0; x <= x1; x++)
		HC_Pixel(x, y, on);
}

static void HC_VLine(uint8_t x, uint8_t y0, uint8_t y1, bool on)
{
	if (y0 > y1) {
		uint8_t t = y0;
		y0 = y1;
		y1 = t;
	}
	for (uint8_t y = y0; y <= y1; y++)
		HC_Pixel(x, y, on);
}

static void HC_Rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on)
{
	HC_HLine(x0, x1, y0, on);
	HC_HLine(x0, x1, y1, on);
	HC_VLine(x0, y0, y1, on);
	HC_VLine(x1, y0, y1, on);
}

static void HC_ClearRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	for (uint8_t y = y0; y <= y1; y++)
		for (uint8_t x = x0; x <= x1; x++)
			HC_Pixel(x, y, false);
}

/* L-shaped dashed peek of the back card (offset down-right).
 * Front later clears its interior, so only the protruding L remains:
 *   - bottom strip under front
 *   - right strip beside front
 * Left/top of that L must meet the front card edges. */
static void HC_DrawBackPeekFrame(void)
{
	const uint8_t bx0 = (uint8_t)(FRONT_X0 + BACK_OX);
	const uint8_t by0 = (uint8_t)(FRONT_Y0 + BACK_OY);
	const uint8_t bx1 = (uint8_t)(FRONT_X1 + BACK_OX);
	const uint8_t by1 = (uint8_t)(FRONT_Y1 + BACK_OY);

	/* Left edge of bottom strip → up to front's bottom edge */
	for (uint8_t y = (uint8_t)(FRONT_Y1 + 1); y <= by1; y += 2)
		HC_Pixel(bx0, y, true);

	/* Bottom edge */
	for (uint8_t x = bx0; x <= bx1; x += 2)
		HC_Pixel(x, by1, true);

	/* Right edge (below front + beside front) */
	for (uint8_t y = by0; y <= by1; y += 2) {
		if (y > FRONT_Y1 || bx1 > FRONT_X1)
			HC_Pixel(bx1, y, true);
	}

	/* Top edge of right strip → left to front's right edge */
	for (uint8_t x = (uint8_t)(FRONT_X1 + 1); x <= bx1; x += 2)
		HC_Pixel(x, by0, true);
}

/* Dondji dual-VFO small font (u8g2_font_5_tr) at absolute screen y (glyph top). */
static void HC_Small(const char *s, uint8_t x, uint8_t y)
{
	if (y < 8)
		HomeCardFont_DrawSmallTextStatus(s, x, y, true);
	else
		HomeCardFont_DrawSmallText(s, x, (uint8_t)(y - 8), true);
}

static void HC_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t err = dx + dy;
	for (;;) {
		if (x0 >= 0 && y0 >= 0 && x0 < HC_W && y0 < HC_H)
			HC_Pixel((uint8_t)x0, (uint8_t)y0, true);
		if (x0 == x1 && y0 == y1)
			break;
		const int16_t e2 = err * 2;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

/* Midpoint circle points; skip SE quadrant (freq cutout). */
static void HC_GaugePlot8(uint8_t cx, uint8_t cy, int16_t x, int16_t y)
{
	const int16_t pts[8][2] = {
		{  x,  y }, {  y,  x }, { -y,  x }, { -x,  y },
		{ -x, -y }, { -y, -x }, {  y, -x }, {  x, -y },
	};
	for (uint8_t i = 0; i < 8; i++) {
		if (pts[i][0] > 0 && pts[i][1] > 0)
			continue;
		HC_Pixel((uint8_t)(cx + pts[i][0]), (uint8_t)(cy + pts[i][1]), true);
	}
}

static void HC_DrawCircleMid(uint8_t cx, uint8_t cy, uint8_t r)
{
	int16_t x = (int16_t)r;
	int16_t y = 0;
	int16_t err = 1 - x;

	while (x >= y) {
		HC_GaugePlot8(cx, cy, x, y);
		y++;
		if (err < 0)
			err += 2 * y + 1;
		else {
			x--;
			err += 2 * (y - x) + 1;
		}
	}
}

/* Smooth 3/4 gauge: midpoint outlines at r and r-1, plus light fill in the ring. */
static void HC_DrawGaugeArc(uint8_t cx, uint8_t cy, uint8_t r)
{
	const int16_t r_out = (int16_t)r;
	const int16_t r_in  = (int16_t)r - (int16_t)GAUGE_THICK;
	const int16_t r2_out = r_out * r_out;
	const int16_t r2_mid = (r_out - 1) * (r_out - 1);
	const int16_t r2_in  = (r_in > 0) ? (r_in * r_in) : 0;

	/* Crisp outer/inner strokes */
	HC_DrawCircleMid(cx, cy, r);
	if (r > 1)
		HC_DrawCircleMid(cx, cy, (uint8_t)(r - 1));

	/* Soft-fill only the mid band so the rim looks even, not blocky */
	for (int16_t dy = -r_out; dy <= r_out; dy++) {
		for (int16_t dx = -r_out; dx <= r_out; dx++) {
			if (dx > 0 && dy > 0)
				continue;
			const int16_t d2 = (int16_t)(dx * dx + dy * dy);
			if (d2 > r2_out || d2 < r2_in)
				continue;
			/* Prefer near-mid radius pixels to thicken without staircase blobs */
			if (d2 < r2_mid && d2 > r2_in)
				HC_Pixel((uint8_t)(cx + dx), (uint8_t)(cy + dy), true);
		}
	}

	/* Finished cut ends at south & east rim */
	HC_Pixel((uint8_t)(cx + r), cy, true);
	HC_Pixel((uint8_t)(cx + r - 1), cy, true);
	HC_Pixel(cx, (uint8_t)(cy + r), true);
	HC_Pixel(cx, (uint8_t)(cy + r - 1), true);
}

/*
 * Tick tips for GAUGE_R=18: angles 90°..360° step 33.75°.
 * Outer r=14, inner r=11 — both strictly inside inner rim (r-thick=16).
 */
static const int8_t hc_tick_ox[9] = {  0, -8,-13,-13,-10, -3,  5, 11, 14};
static const int8_t hc_tick_oy[9] = { 14, 11,  5, -3, -9,-13,-13, -8,  0};
static const int8_t hc_tick_ix[9] = {  0, -6,-10,-10, -8, -2,  4,  9, 11};
static const int8_t hc_tick_iy[9] = { 11,  9,  4, -2, -7,-10,-10, -6,  0};

static void HC_DrawTicks(uint8_t cx, uint8_t cy)
{
	for (uint8_t n = 0; n < 9; n++)
		HC_Line(cx + hc_tick_ox[n], cy + hc_tick_oy[n],
			cx + hc_tick_ix[n], cy + hc_tick_iy[n]);
}

static void HC_DrawNeedle(uint8_t cx, uint8_t cy, uint8_t s_level)
{
	if (s_level > 9)
		s_level = 9;
	const uint8_t idx = (s_level == 0) ? 0 : (uint8_t)(s_level - 1);
	/* Needle tip uses inner tick radius — stays clear of the rim */
	HC_Line(cx, cy, cx + hc_tick_ix[idx], cy + hc_tick_iy[idx]);
	/* Small hub */
	HC_Pixel(cx, cy, true);
	HC_Pixel((uint8_t)(cx + 1), cy, true);
	HC_Pixel(cx, (uint8_t)(cy + 1), true);
	HC_Pixel((uint8_t)(cx - 1), cy, true);
	HC_Pixel(cx, (uint8_t)(cy - 1), true);
}

/* Bold 7-segment digit 8×12 (2px strokes) */
static const uint8_t seg_mask[10] = {
	/* a b c d e f g */
	0x3F, /* 0 abcdef */
	0x06, /* 1 bc */
	0x5B, /* 2 abdeg */
	0x4F, /* 3 abcdg */
	0x66, /* 4 bcfg */
	0x6D, /* 5 acdfg */
	0x7D, /* 6 acdefg */
	0x07, /* 7 abc */
	0x7F, /* 8 */
	0x6F, /* 9 abcdfg */
};

static void HC_Seg7(uint8_t x, uint8_t y, char ch)
{
	if (ch < '0' || ch > '9')
		return;
	const uint8_t m = seg_mask[ch - '0'];
	/* a */
	if (m & 0x01) {
		HC_HLine(x + 1, x + 5, y, true);
		HC_HLine(x + 1, x + 5, (uint8_t)(y + 1), true);
	}
	/* b */
	if (m & 0x02) {
		HC_VLine((uint8_t)(x + 6), (uint8_t)(y + 1), (uint8_t)(y + 5), true);
		HC_VLine((uint8_t)(x + 5), (uint8_t)(y + 1), (uint8_t)(y + 5), true);
	}
	/* c */
	if (m & 0x04) {
		HC_VLine((uint8_t)(x + 6), (uint8_t)(y + 7), (uint8_t)(y + 11), true);
		HC_VLine((uint8_t)(x + 5), (uint8_t)(y + 7), (uint8_t)(y + 11), true);
	}
	/* d */
	if (m & 0x08) {
		HC_HLine(x + 1, x + 5, (uint8_t)(y + 12), true);
		HC_HLine(x + 1, x + 5, (uint8_t)(y + 11), true);
	}
	/* e */
	if (m & 0x10) {
		HC_VLine(x, (uint8_t)(y + 7), (uint8_t)(y + 11), true);
		HC_VLine((uint8_t)(x + 1), (uint8_t)(y + 7), (uint8_t)(y + 11), true);
	}
	/* f */
	if (m & 0x20) {
		HC_VLine(x, (uint8_t)(y + 1), (uint8_t)(y + 5), true);
		HC_VLine((uint8_t)(x + 1), (uint8_t)(y + 1), (uint8_t)(y + 5), true);
	}
	/* g */
	if (m & 0x40) {
		HC_HLine(x + 1, x + 5, (uint8_t)(y + 6), true);
		HC_HLine(x + 1, x + 5, (uint8_t)(y + 5), true);
	}
}

static void HC_DrawFreqLed(uint8_t x, uint8_t y, uint32_t hz)
{
	char buf[12];
	/* Full radio format: 145.55000 */
	sprintf(buf, "%03u.%05u", hz / 100000u, hz % 100000u);
	uint8_t px = x;
	for (uint8_t i = 0; buf[i]; i++) {
		if (buf[i] == '.') {
			HC_Pixel(px, (uint8_t)(y + 11), true);
			HC_Pixel((uint8_t)(px + 1), (uint8_t)(y + 11), true);
			HC_Pixel(px, (uint8_t)(y + 10), true);
			HC_Pixel((uint8_t)(px + 1), (uint8_t)(y + 10), true);
			px = (uint8_t)(px + 3);
		} else {
			HC_Seg7(px, y, buf[i]);
			px = (uint8_t)(px + 8);
		}
	}
}

/* Compact battery: 9×5 outline + tip. Fill tracks gBatteryDisplayLevel (0..7). */
static void HC_DrawMiniBattery(uint8_t x, uint8_t y)
{
	uint8_t level = gBatteryDisplayLevel;
	if (level < 2 && gLowBatteryBlink)
		return;

	/* body 9×5 (1px shorter than before), tip on the right */
	HC_Rect(x, y, (uint8_t)(x + 8), (uint8_t)(y + 4), true);
	HC_VLine((uint8_t)(x + 9), (uint8_t)(y + 1), (uint8_t)(y + 3), true);

	/* levels: 0/1 empty (blink), 2 empty outline, 3..6 → 1..4 bars, 7 full */
	uint8_t bars = 0;
	if (level >= 7)
		bars = 4;
	else if (level > 2)
		bars = (uint8_t)MIN(4, level - 2);

	for (uint8_t i = 0; i < bars; i++) {
		const uint8_t bx = (uint8_t)(x + 1 + i * 2);
		HC_VLine(bx, (uint8_t)(y + 1), (uint8_t)(y + 3), true);
	}
}

static uint8_t HC_GetSLevel(void)
{
	int16_t rssi_dBm =
		BK4819_GetRSSI_dBm()
#ifdef ENABLE_AM_FIX
		+ ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif
		+ dBmCorrTable[gRxVfo->Band];

	uint8_t s_level;
	if (rssi_dBm >= -93)
		s_level = 9;
	else if (rssi_dBm < -141)
		s_level = 0;
	else
		s_level = (uint8_t)((rssi_dBm + 147) / 6);
	return s_level;
}

/* RX/TX tone label: RC/TC (CTCSS), RD/TD (DCS). Empty if unset. */
static bool HC_FormatTone(const FREQ_Config_t *p, bool tx, char *out, uint8_t out_len)
{
	out[0] = 0;
	if (out_len < 8)
		return false;

	switch (p->CodeType) {
	case CODE_TYPE_CONTINUOUS_TONE:
		sprintf(out, "%cC%u.%u", tx ? 'T' : 'R',
			CTCSS_Options[p->Code] / 10, CTCSS_Options[p->Code] % 10);
		return true;
	case CODE_TYPE_DIGITAL:
		sprintf(out, "%cD%03oN", tx ? 'T' : 'R', DCS_Options[p->Code]);
		return true;
	case CODE_TYPE_REVERSE_DIGITAL:
		sprintf(out, "%cD%03oI", tx ? 'T' : 'R', DCS_Options[p->Code]);
		return true;
	default:
		return false;
	}
}

static void HC_FormatPower(const VFO_Info_t *vfo, char *out)
{
	static const char pwr[][3] = {"L1", "L2", "L3", "L4", "L5", "M", "H"};
	uint8_t p = vfo->OUTPUT_POWER % 8;
	if (p == OUTPUT_POWER_USER)
		p = gSetting_set_pwr;
	else if (p > 0)
		p--;
	if (p > 6)
		p = 6;
	strcpy(out, pwr[p]);
}

static void HC_GetName(uint8_t vfo_num, char *name, uint8_t name_len)
{
	const uint16_t ch = gEeprom.ScreenChannel[vfo_num];
	if (IS_MR_CHANNEL(ch)) {
		SETTINGS_FetchChannelName(name, ch);
		if (name[0] == 0 || name[0] == 0xFF)
			sprintf(name, "CH-%u", (unsigned)(ch + 1));
	} else {
		sprintf(name, "VFO %c", (vfo_num == 0) ? 'A' : 'B');
	}
	(void)name_len;
}

static void HC_GetChannelLabel(uint8_t vfo_num, char *out, uint8_t out_len)
{
	const uint16_t ch = gEeprom.ScreenChannel[vfo_num];
	if (IS_MR_CHANNEL(ch))
		sprintf(out, "%u", (unsigned)(ch + 1));
	else if (IS_FREQ_CHANNEL(ch))
		sprintf(out, "F%u", (unsigned)(1 + ch - FREQ_CHANNEL_FIRST));
	else
		strcpy(out, "-");
	(void)out_len;
}

static void HC_FillDtmf(char *out, uint8_t out_len)
{
	out[0] = 0;
	(void)out_len;
#ifdef ENABLE_DTMF_CALLING
	char Contact[16];
	if (gDTMF_InputMode) {
		sprintf(out, ">%s", gDTMF_InputBox);
		return;
	}
	if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT) {
		strcpy(out, (gDTMF_State == DTMF_STATE_CALL_OUT_RSP) ? "CALL OUT(RSP)" : "CALL OUT");
		return;
	}
	if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED || gDTMF_CallState == DTMF_CALL_STATE_RECEIVED_STAY) {
		sprintf(out, "CALL FRM:%s",
			DTMF_FindContact(gDTMF_Caller, Contact) ? Contact : gDTMF_Caller);
		return;
	}
	if (gDTMF_IsTx) {
		strcpy(out, (gDTMF_State == DTMF_STATE_TX_SUCC) ? "DTMF TX(SUCC)" : "DTMF TX");
		return;
	}
#endif
	if (gDTMF_RX_live[0] != 0 && gDTMF_RX_live_timeout > 0)
		strcpy(out, gDTMF_RX_live);
}

static void HC_DrawBackPeek(uint8_t vfo_num)
{
	char name[12];
	char line[28];
	const VFO_Info_t *vfo = &gEeprom.VfoInfo[vfo_num];
	const uint32_t freq = vfo->pRX->Frequency;

	HC_DrawBackPeekFrame();

	HC_GetName(vfo_num, name, sizeof(name));
	/* keep name short for one line */
	name[8] = 0;
	sprintf(line, "%s %u.%05u %s",
		name,
		freq / 100000u, freq % 100000u,
		gModulationStr[vfo->Modulation]);
	HC_Small(line, 10, 56);
}

static void HC_DrawFront(uint8_t vfo_num)
{
	char str[40];
	char rx_tone[10];
	char tx_tone[10];
	char pwr[4];
	const VFO_Info_t *vfo = &gEeprom.VfoInfo[vfo_num];
	const bool is_tx = (gCurrentFunction == FUNCTION_TRANSMIT);
	const uint32_t freq = is_tx ? vfo->pTX->Frequency : vfo->pRX->Frequency;
	const uint8_t s_level = (!is_tx && FUNCTION_IsRx()) ? HC_GetSLevel() : 0;

	/* Cover anything from the back card that falls under the front. */
	HC_ClearRect(FRONT_X0, FRONT_Y0, FRONT_X1, FRONT_Y1);
	HC_HLine(FRONT_X0, FRONT_X1, FRONT_Y0, true);
	HC_HLine(FRONT_X0, FRONT_X1, FRONT_Y1, true);
	HC_VLine(FRONT_X0, FRONT_Y0, FRONT_Y1, true);
	HC_VLine(FRONT_X1, FRONT_Y0, FRONT_Y1, true);

	/* S-meter text */
	sprintf(str, "S%u", s_level);
	HC_Small(str, 5, 3);

	/* VFO letter + channel # + battery + %/V */
	str[0] = (vfo_num == 0) ? 'A' : 'B';
	str[1] = 0;
	HC_Small(str, 60, 3);
	HC_GetChannelLabel(vfo_num, str, sizeof(str));
	HC_Small(str, 68, 3);
	HC_DrawMiniBattery(82, 3);
	switch (gSetting_battery_text) {
	case 1: {
		const uint16_t voltage = MIN(gBatteryVoltageAverage, 999);
		sprintf(str, "%u.%02u", voltage / 100, voltage % 100);
		break;
	}
	case 2:
		sprintf(str, "%02u%%", BATTERY_VoltsToPercent(gBatteryVoltageAverage));
		break;
	default:
		str[0] = 0;
		break;
	}
	if (str[0])
		HC_Small(str, 94, 3);

	/* gauge */
	HC_DrawGaugeArc(GAUGE_CX, GAUGE_CY, GAUGE_R);
	HC_DrawTicks(GAUGE_CX, GAUGE_CY);
	HC_DrawNeedle(GAUGE_CX, GAUGE_CY, s_level);

	/* DTMF (parsed/live) — tiny u8g2 small font */
	HC_FillDtmf(str, sizeof(str));
	if (str[0])
		HC_Small(str, 50, 10);

	/* channel name / VFO — half of big font (gFontSmall) */
	HC_GetName(vfo_num, str, sizeof(str));
	str[10] = 0;
#ifdef ENABLE_SMALL_BOLD
	UI_PrintStringSmallBold(str, 48, 0, 1);
#else
	UI_PrintStringSmallNormal(str, 48, 0, 1);
#endif

	/* frequency LED below name: full XXX.XXXXX, bold 7-seg */
	HC_DrawFreqLed(34, 28, freq);

	/* info: power, RX/TX tones, mod, SQL, bandwidth (above screen-bottom mic bar) */
	{
		const char *bw;
		char *p;
#ifdef ENABLE_FEAT_F4HWN_NARROWER
		if (vfo->CHANNEL_BANDWIDTH == BANDWIDTH_NARROW && gSetting_set_nfm == 1)
			bw = "N+";
		else
#endif
		bw = (vfo->CHANNEL_BANDWIDTH == BANDWIDTH_NARROW) ? "N" : "W";

		HC_FormatPower(vfo, pwr);
		const bool has_rx = HC_FormatTone(&vfo->freq_config_RX, false, rx_tone, sizeof(rx_tone));
		const bool has_tx = HC_FormatTone(&vfo->freq_config_TX, true, tx_tone, sizeof(tx_tone));

		p = str;
		p += sprintf(p, "%s", pwr);
		if (has_rx)
			p += sprintf(p, " %s", rx_tone);
		if (has_tx)
			p += sprintf(p, " %s", tx_tone);
		sprintf(p, " %s SQL%u %s",
			gModulationStr[vfo->Modulation],
			gEeprom.SQUELCH_LEVEL, bw);
		HC_Small(str, 6, 48);
	}
}

void UI_DisplayHomeCard(void)
{
	UI_DisplayClear();
	UI_StatusClear();

	if (gLowBattery && !gLowBatteryConfirmed) {
		UI_DisplayPopup("LOW BATTERY");
		ST7565_BlitStatusLine();
		ST7565_BlitFullScreen();
		return;
	}

	const uint8_t front = gEeprom.TX_VFO & 1u;
	const uint8_t back  = front ^ 1u;
	const bool dual = (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
		|| (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF);

	if (dual)
		HC_DrawBackPeek(back);

	HC_DrawFront(front);

	ST7565_BlitStatusLine();
	ST7565_BlitFullScreen();

#ifdef ENABLE_FEAT_F4HWN_AUDIO_SCOPE
	/* Bottom strip: audio scope only (not the classic level bar). */
	if (gCurrentFunction == FUNCTION_TRANSMIT && gSetting_mic_bar)
		UI_DisplayAudioScope();
#endif
}

#endif /* ENABLE_FEAT_F4HWN */
