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
#ifdef ENABLE_BK1080
#include "driver/bk1080.h"
#endif
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

/* Ellipse: +5px wider to the left (left rim 10→5), height unchanged.
 * Wider RX corrects the tall-looking circle on the ST7565 aspect. */
#define GAUGE_CX        25
#define GAUGE_CY        24
#define GAUGE_RX        20
#define GAUGE_RY        18
#define GAUGE_THICK     2

/* Displayed needle; rises instantly, falls 1 S-unit per 500ms tick. */
static uint8_t hc_needle;
static uint8_t hc_needle_target;

/* Draw offset for A/B swap animation (applied in HC_Pixel / HC_Small). */
static int8_t hc_ox;
static int8_t hc_oy;

/* Manual A/B swap: whole front card slides up; layout under it is final home.
 * Needle frozen until the slide finishes. */
#define HC_ANIM_STEPS           9u
#define HC_ANIM_TICKS_PER_STEP  5u  /* 50ms × 9 ≈ 450ms */
static bool    hc_anim_active;
static bool    hc_needle_frozen;
static uint8_t hc_anim_step;
static uint8_t hc_anim_tick;
static uint8_t hc_anim_out; /* VFO leaving (slides up) */
static uint8_t hc_anim_in;  /* VFO arriving (stays as new front) */

static void HC_PixelXY(int16_t x, int16_t y, bool on)
{
	x += hc_ox;
	y += hc_oy;
	if (x < 0 || y < 0 || x >= HC_W || y >= HC_H)
		return;
	if (y < 8)
		PutPixelStatus((uint8_t)x, (uint8_t)y, on);
	else
		PutPixel((uint8_t)x, (uint8_t)(y - 8), on);
}

static void HC_Pixel(uint8_t x, uint8_t y, bool on)
{
	HC_PixelXY((int16_t)x, (int16_t)y, on);
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
	const int16_t px = (int16_t)x + hc_ox;
	const int16_t py = (int16_t)y + hc_oy;

	if (px < 0 || py < 0 || px >= HC_W || py >= HC_H)
		return;

	/* Keep card-relative layout: same (x,y) → screen (x+ox, y+oy). */
	if (py < 8)
		HomeCardFont_DrawSmallTextStatus(s, (uint8_t)px, (uint8_t)py, true);
	else
		HomeCardFont_DrawSmallText(s, (uint8_t)px, (uint8_t)(py - 8), true);
}

static void HC_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t err = dx + dy;
	for (;;) {
		HC_PixelXY(x0, y0, true);
		if (x0 == x1 && y0 == y1)
			break;
		const int16_t e2 = err * 2;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

/* Midpoint ellipse points; skip SE quadrant (freq cutout). */
static void HC_GaugePlot4(uint8_t cx, uint8_t cy, int16_t x, int16_t y)
{
	const int16_t pts[4][2] = {
		{  x,  y }, { -x,  y }, { -x, -y }, {  x, -y },
	};
	for (uint8_t i = 0; i < 4; i++) {
		if (pts[i][0] > 0 && pts[i][1] > 0)
			continue;
		HC_Pixel((uint8_t)(cx + pts[i][0]), (uint8_t)(cy + pts[i][1]), true);
	}
}

static void HC_DrawEllipseMid(uint8_t cx, uint8_t cy, uint8_t rx, uint8_t ry)
{
	if (rx == 0 || ry == 0)
		return;

	int32_t x = 0;
	int32_t y = (int32_t)ry;
	const int32_t rx2 = (int32_t)rx * (int32_t)rx;
	const int32_t ry2 = (int32_t)ry * (int32_t)ry;
	int32_t px = 0;
	int32_t py = 2 * rx2 * y;
	int32_t p = ry2 - (rx2 * (int32_t)ry) + (rx2 / 4);

	while (px < py) {
		HC_GaugePlot4(cx, cy, (int16_t)x, (int16_t)y);
		x++;
		px += 2 * ry2;
		if (p < 0)
			p += ry2 + px;
		else {
			y--;
			py -= 2 * rx2;
			p += ry2 + px - py;
		}
	}

	p = ry2 * (x + 1) * (x + 1) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
	while (y >= 0) {
		HC_GaugePlot4(cx, cy, (int16_t)x, (int16_t)y);
		y--;
		py -= 2 * rx2;
		if (p > 0)
			p += rx2 - py;
		else {
			x++;
			px += 2 * ry2;
			p += rx2 - py + px;
		}
	}
}

/* Smooth 3/4 gauge: elliptical outlines + light fill in the ring. */
static void HC_DrawGaugeArc(uint8_t cx, uint8_t cy, uint8_t rx, uint8_t ry)
{
	const int32_t rx_out = (int32_t)rx;
	const int32_t ry_out = (int32_t)ry;
	const int32_t rx_in  = (int32_t)rx - (int32_t)GAUGE_THICK;
	const int32_t ry_in  = (int32_t)ry - (int32_t)GAUGE_THICK;
	const int32_t rx2_out = rx_out * rx_out;
	const int32_t ry2_out = ry_out * ry_out;
	const int32_t rx2_mid = (rx_out - 1) * (rx_out - 1);
	const int32_t ry2_mid = (ry_out - 1) * (ry_out - 1);
	const int32_t rx2_in  = (rx_in > 0) ? (rx_in * rx_in) : 0;
	const int32_t ry2_in  = (ry_in > 0) ? (ry_in * ry_in) : 0;

	HC_DrawEllipseMid(cx, cy, rx, ry);
	if (rx > 1 && ry > 1)
		HC_DrawEllipseMid(cx, cy, (uint8_t)(rx - 1), (uint8_t)(ry - 1));

	/* Skip soft-fill while a card is offset-sliding (CPU); outline+ticks still match layout. */
	if (hc_ox != 0 || hc_oy != 0)
		return;

	/* Soft-fill mid band: dx²/rx² + dy²/ry² in [in, out] */
	for (int16_t dy = (int16_t)(-ry_out); dy <= (int16_t)ry_out; dy++) {
		for (int16_t dx = (int16_t)(-rx_out); dx <= (int16_t)rx_out; dx++) {
			if (dx > 0 && dy > 0)
				continue;
			const int32_t dx2 = (int32_t)dx * (int32_t)dx;
			const int32_t dy2 = (int32_t)dy * (int32_t)dy;
			/* Outside outer ellipse? */
			if (dx2 * ry2_out + dy2 * rx2_out > rx2_out * ry2_out)
				continue;
			/* Inside inner ellipse? */
			if (rx2_in > 0 && ry2_in > 0 &&
			    dx2 * ry2_in + dy2 * rx2_in < rx2_in * ry2_in)
				continue;
			/* Prefer mid-band pixels */
			if (dx2 * ry2_mid + dy2 * rx2_mid < rx2_mid * ry2_mid &&
			    (rx2_in == 0 || dx2 * ry2_in + dy2 * rx2_in > rx2_in * ry2_in))
				HC_Pixel((uint8_t)(cx + dx), (uint8_t)(cy + dy), true);
		}
	}

	/* Finished cut ends at south & east rim */
	HC_Pixel((uint8_t)(cx + rx), cy, true);
	HC_Pixel((uint8_t)(cx + rx - 1), cy, true);
	HC_Pixel(cx, (uint8_t)(cy + ry), true);
	HC_Pixel(cx, (uint8_t)(cy + ry - 1), true);
}

/*
 * Tick tips for GAUGE_RX=20, GAUGE_RY=18: angles 90°..360° step 33.75°.
 * Outer ≈14/18 of rim, inner ≈11/18 — both inside inner rim.
 */
static const int8_t hc_tick_ox[9] = {  0, -9,-14,-15,-11, -3,  6, 13, 16};
static const int8_t hc_tick_oy[9] = { 14, 12,  5, -3,-10,-14,-13, -8,  0};
static const int8_t hc_tick_ix[9] = {  0, -7,-11,-12, -9, -2,  5, 10, 12};
static const int8_t hc_tick_iy[9] = { 11,  9,  4, -2, -8,-11,-10, -6,  0};

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
#ifdef ENABLE_BK1080
	/*
	 * WFM uses BK1080 REG10 RSSI (dBμV). Datasheet ceiling is ~75, but on a
	 * stock HT antenna clear locals are typically ~15–30 and stereo blend
	 * starts near 31 — so map a practical full-scale to S9, not 75.
	 */
	if (gRxVfo && gRxVfo->Modulation == MODULATION_WFM) {
		const uint8_t rssi = BK1080_GetRSSI();
		enum { WFM_RSSI_S9 = 28 }; /* dBμV → S9 */
		if (rssi >= WFM_RSSI_S9)
			return 9;
		return (uint8_t)((rssi * 9u) / WFM_RSSI_S9);
	}
#endif
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

/* DTMF line left edge; text must stay inside FRONT_X1. */
#define HC_DTMF_X  50

/* Drop leading chars until the string fits left of the front-card right border. */
static void HC_SmallFit(const char *s, uint8_t x, uint8_t y)
{
	if (s == NULL || s[0] == 0)
		return;

	const uint8_t max_w = (FRONT_X1 > x) ? (uint8_t)(FRONT_X1 - x) : 0;
	const char *p = s;

	while (p[0] != 0 && HomeCardFont_GetSmallTextWidth(p) > max_w)
		p++;

	if (p[0])
		HC_Small(p, x, y);
}

static void HC_FillDtmf(char *out, uint8_t out_len)
{
	out[0] = 0;
	if (out_len == 0)
		return;

	/* Yan ID CALL SIGN takes priority over live DTMF / call status. */
	if (gYanId_RX[0] != 0 && gYanId_RX_timeout > 0) {
		const uint8_t max_w = (FRONT_X1 > HC_DTMF_X) ? (uint8_t)(FRONT_X1 - HC_DTMF_X) : 0;
		const char *id = gYanId_RX;

		for (;;) {
			snprintf(out, out_len, "CALL SIGN:%s", id);
			if (HomeCardFont_GetSmallTextWidth(out) <= max_w || id[0] == 0)
				break;
			id++;
		}
		return;
	}

#ifdef ENABLE_DTMF_CALLING
	char Contact[16];
	if (gDTMF_InputMode) {
		snprintf(out, out_len, ">%s", gDTMF_InputBox);
		return;
	}
	if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT) {
		strcpy(out, (gDTMF_State == DTMF_STATE_CALL_OUT_RSP) ? "CALL OUT(RSP)" : "CALL OUT");
		return;
	}
	if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED || gDTMF_CallState == DTMF_CALL_STATE_RECEIVED_STAY) {
		snprintf(out, out_len, "CALL FRM:%s",
			DTMF_FindContact(gDTMF_Caller, Contact) ? Contact : gDTMF_Caller);
		return;
	}
	if (gDTMF_IsTx) {
		strcpy(out, (gDTMF_State == DTMF_STATE_TX_SUCC) ? "DTMF TX(SUCC)" : "DTMF TX");
		return;
	}
#endif
	if (gDTMF_RX_live[0] != 0 && gDTMF_RX_live_timeout > 0) {
		/* Keep "DTMF:" fixed; scroll decoded digits from the left when too wide. */
		const uint8_t max_w = (FRONT_X1 > HC_DTMF_X) ? (uint8_t)(FRONT_X1 - HC_DTMF_X) : 0;
		const char *live = gDTMF_RX_live;

		for (;;) {
			snprintf(out, out_len, "DTMF:%s", live);
			if (HomeCardFont_GetSmallTextWidth(out) <= max_w || live[0] == 0)
				break;
			live++;
		}
	}
}

static void HC_DrawBackPeek(uint8_t vfo_num)
{
	char name[12];
	char line[28];
	const VFO_Info_t *vfo = &gEeprom.VfoInfo[vfo_num];
	const uint32_t freq = vfo->freq_config_RX.Frequency;

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
	const bool is_tx = (gCurrentFunction == FUNCTION_TRANSMIT)
		&& ((gEeprom.TX_VFO & 1u) == vfo_num);
	const uint32_t freq = is_tx ? vfo->freq_config_TX.Frequency : vfo->freq_config_RX.Frequency;
	const bool meter_vfo = ((gEeprom.RX_VFO & 1u) == vfo_num);
	/* WFM never enters FUNCTION_RECEIVE (BK4819 squelch ignored), so always meter. */
	const bool show_s =
		!hc_needle_frozen &&
		meter_vfo &&
		!is_tx &&
		(FUNCTION_IsRx()
#ifdef ENABLE_BK1080
		 || (gRxVfo && gRxVfo->Modulation == MODULATION_WFM)
#endif
		);
	const uint8_t s_level = show_s ? HC_GetSLevel() : 0;

	/* Instant rise, slow fall — frozen during A/B card slide. */
	if (!hc_needle_frozen && meter_vfo) {
		hc_needle_target = s_level;
		if (hc_needle_target >= hc_needle)
			hc_needle = hc_needle_target;
	}

	/* Cover anything from the back card that falls under the front. */
	HC_ClearRect(FRONT_X0, FRONT_Y0, FRONT_X1, FRONT_Y1);
	HC_HLine(FRONT_X0, FRONT_X1, FRONT_Y0, true);
	HC_HLine(FRONT_X0, FRONT_X1, FRONT_Y1, true);
	HC_VLine(FRONT_X0, FRONT_Y0, FRONT_Y1, true);
	HC_VLine(FRONT_X1, FRONT_Y0, FRONT_Y1, true);

	/* S-meter text follows the displayed needle */
	sprintf(str, "S%u", hc_needle);
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
	HC_DrawGaugeArc(GAUGE_CX, GAUGE_CY, GAUGE_RX, GAUGE_RY);
	HC_DrawTicks(GAUGE_CX, GAUGE_CY);
	HC_DrawNeedle(GAUGE_CX, GAUGE_CY, hc_needle);

	/* DTMF (parsed/live) — tiny u8g2 small font; scroll if past card edge */
	HC_FillDtmf(str, sizeof(str));
	if (str[0])
		HC_SmallFit(str, HC_DTMF_X, 10);

	/* channel name / VFO — half of big font (gFontSmall); offset path uses small font */
	HC_GetName(vfo_num, str, sizeof(str));
	str[10] = 0;
	if (hc_ox != 0 || hc_oy != 0) {
		HC_Small(str, 48, 16);
	} else {
#ifdef ENABLE_SMALL_BOLD
		UI_PrintStringSmallBold(str, 48, 0, 1);
#else
		UI_PrintStringSmallNormal(str, 48, 0, 1);
#endif
	}

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

	if (gScreenToDisplay != DISPLAY_MAIN && hc_anim_active)
		hc_anim_active = false;

	const uint8_t front = gEeprom.TX_VFO & 1u;
	const uint8_t back  = front ^ 1u;
	const bool dual = (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF)
		|| (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF);

	if (hc_anim_active && dual) {
		/* Top card slides up from its current slot (offset 0 → off the top). */
		const int16_t card_h = (int16_t)(FRONT_Y1 - FRONT_Y0 + 1);
		const int8_t out_dy =
			(int8_t)(-(card_h * (int16_t)hc_anim_step) / (int16_t)HC_ANIM_STEPS);

		hc_ox = 0;
		hc_oy = 0;
		HC_DrawFront(hc_anim_in);   /* new card underneath */

		hc_oy = out_dy;             /* 0 on first frame = current position */
		HC_DrawFront(hc_anim_out);  /* current top card moving up */
		hc_oy = 0;
	} else {
		hc_anim_active   = false;
		hc_needle_frozen = false;
		hc_anim_step     = 0;
		hc_anim_tick     = 0;
		hc_ox = 0;
		hc_oy = 0;

		/* Normal home: front card + back peek (name / freq / modulation). */
		if (dual)
			HC_DrawBackPeek(back);

		HC_DrawFront(front);
	}

	ST7565_BlitStatusLine();
	ST7565_BlitFullScreen();

#ifdef ENABLE_FEAT_F4HWN_AUDIO_SCOPE
	/* Bottom strip: audio scope only (not the classic level bar). */
	if (gCurrentFunction == FUNCTION_TRANSMIT && gSetting_mic_bar)
		UI_DisplayAudioScope();
#endif
}

void UI_HomeCard_StartVfoSwapAnim(uint8_t outgoing_vfo)
{
	hc_anim_out      = outgoing_vfo & 1u;
	hc_anim_in       = hc_anim_out ^ 1u;
	hc_anim_step     = 0;
	hc_anim_tick     = 0;
	hc_needle_frozen = true;
	hc_anim_active   = true;
	gUpdateDisplay   = true;
}

bool UI_HomeCard_CancelVfoSwapAnim(void)
{
	if (!hc_anim_active)
		return false;
	hc_anim_active   = false;
	hc_needle_frozen = false;
	hc_anim_step     = 0;
	hc_anim_tick     = 0;
	hc_ox = 0;
	hc_oy = 0;
	return true;
}

bool UI_HomeCard_IsVfoSwapAnimActive(void)
{
	return hc_anim_active;
}

bool UI_HomeCard_TimeSlice10ms(void)
{
	if (!hc_anim_active)
		return false;

	if (gScreenToDisplay != DISPLAY_MAIN) {
		hc_anim_active   = false;
		hc_needle_frozen = false;
		hc_anim_step     = 0;
		hc_anim_tick     = 0;
		return false;
	}

	if (++hc_anim_tick < HC_ANIM_TICKS_PER_STEP)
		return false;
	hc_anim_tick = 0;

	if (hc_anim_step < HC_ANIM_STEPS)
		hc_anim_step++;

	if (hc_anim_step >= HC_ANIM_STEPS) {
		/* Snap to normal home; needle may update again on following draws. */
		hc_anim_active   = false;
		hc_needle_frozen = false;
		hc_anim_step     = 0;
		hc_anim_tick     = 0;
		hc_ox = 0;
		hc_oy = 0;
	}
	return true;
}

bool UI_HomeCard_TimeSlice500ms(void)
{
	uint8_t target = 0;

	if (hc_needle_frozen)
		return false;

	if (gCurrentFunction != FUNCTION_TRANSMIT) {
		if (FUNCTION_IsRx()
#ifdef ENABLE_BK1080
		    || (gRxVfo && gRxVfo->Modulation == MODULATION_WFM)
#endif
		   )
			target = HC_GetSLevel();
	}

	hc_needle_target = target;

	/* Instant rise when signal increases. */
	if (target >= hc_needle) {
		if (hc_needle != target) {
			hc_needle = target;
			return true;
		}
		return false;
	}

	/* Slow fall: 1 S-unit per 500ms — no snap to 0. */
	hc_needle--;
	return true;
}

#endif /* ENABLE_FEAT_F4HWN */
