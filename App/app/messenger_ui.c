/*
 * Mangosteen Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0
 *
 * Project:
 *     山竹 (Mangosteen)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Official Website:
 *     https://ethanyan6.github.io/Mangosteen/
 *
 * The Mangosteen name, logo, and official project identity
 * are protected separately from the source code license.
 */
#include <string.h>
#include <stdio.h>
#include "app/messenger_store.h"
#include "app/messenger_t9.h"
#include "app/messenger_rf.h"
#include "app/messenger_packet.h"
#include "app/messenger_ui.h"
#include "app/pinyin_ime.h"
#include "driver/st7565.h"
#include "driver/py25q16.h"
#include "external/printf/printf.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/ui.h"
#include "ui/menu.h"
#include "misc.h"
#include "font.h"

#ifndef FLASH_FONT16_BASE
#define FLASH_FONT16_BASE  0x0A0000u
#define FLASH_FONT8_BASE   0x0E0000u
#define FONT16_SIZE        32u
#define FONT8_SIZE         8u
#endif

// 汉字字模裁剪参数:跳过左右空白,只渲染实际字形部分
#define CN_LEFT_MARGIN     1u    // 左边空白列数(跳过,保留1像素间隔)
#define CN_RIGHT_MARGIN    3u    // 右边空白列数(跳过)
#define CN_WIDTH           12u   // 裁剪后的汉字宽度(16 - 1 - 3)

extern uint8_t gMsgHomeCursor;
extern uint8_t gMsgCursor;
extern uint8_t gMsgScroll;
extern uint8_t gMsgReadIndex;
extern uint8_t gMsgReadSource;
extern uint8_t gMsgReadScroll;
extern uint8_t gMsgSettingsCursor;
extern char gMsgComposeBuf[];
extern MSG_T9Editor_t gMsgEditor;
extern uint8_t gMsgScreen;
extern uint8_t gMsgDraftMode;   /* 0 normal drafts list, 1 save-into-slot picker, 2 delete-slot confirm */
typedef struct {
    bool used;
    char callsign[MSG_CALLSIGN_EDIT_LEN + 1];
    int8_t rssi;
    uint16_t battery_cv;
    uint16_t age_seconds;
    uint8_t packet_type;
    uint16_t range_session;
} MSG_RangeFound_t;
extern MSG_RangeFound_t gMsgRangeFound[];
extern uint8_t gMsgRangeCount;
extern uint8_t gMsgRangeScroll;
extern uint8_t gMsgRangeStatus;
extern uint16_t gMsgRangeSession;
#define MSG_RANGE_MAX_FOUND 6u

enum { MSG_SCREEN_HOME = 0, MSG_SCREEN_INBOX, MSG_SCREEN_OUTBOX, MSG_SCREEN_DRAFTS, MSG_SCREEN_COMPOSE, MSG_SCREEN_READ, MSG_SCREEN_RANGE };

static bool msg_lang_cn(void)
{
	return gUiLanguage == UI_LANGUAGE_CN;
}

/** English UI key → GB2312 when Chinese. Dispatch on en[0]/en[1..]. */
static const char *msg_label(const char *en)
{
	if (!msg_lang_cn() || en == NULL || en[0] == '\0')
		return en;
	switch (en[0]) {
		case 'M': return (en[1] == 'E') ? "\xD0\xC5\xCF\xA2" : en; /* 信息 */
		case 'I': return "\xCA\xD5\xBC\xFE\xCF\xE4";               /* 收件箱 */
		case 'C': return "\xB1\xE0\xBC\xAD\xD0\xC5\xCF\xA2";       /* 编辑信息 */
		case 'S':
			/* SELECT / SEND / SENT */
			if (en[1] == 'E' && en[2] == 'L')
				return "\xD1\xA1\xD4\xF1";                       /* 选择 */
			if (en[1] == 'E' && en[2] == 'N' && en[3] == 'D' && en[4] == '\0')
				return "\xB7\xA2\xCB\xCD";                       /* 发送 */
			return "\xB7\xA2\xBC\xFE\xCF\xE4";                     /* 发件箱 */
		case 'D': return "\xB2\xDD\xB8\xE5\xCF\xE4";               /* 草稿箱 */
		case 'E':
			if (en[1] == 'M') return "\xBF\xD5";                 /* 空 EMPTY */
			if (en[1] == 'X') return "\xCD\xCB\xB3\xF6";         /* 退出 */
			return en;
		case 'R':
			/* READ / RESEND / RE: / RESULT / RANGE CHECK */
			if (en[1] == 'E') {
				if (en[2] == 'S' && en[3] == 'E')
					return "\xD6\xD8\xB7\xA2";                   /* 重发 */
				if (en[2] == 'S' && en[3] == 'U')
					return "\xBD\xE1\xB9\xFB";                   /* 结果 */
				if (en[2] == 'A')
					return "\xD4\xC4\xB6\xC1";                   /* 阅读 */
				if (en[2] == ':')
					return "\xBB\xD8\xB8\xB4";                   /* 回复 */
				return en;
			}
			if (en[1] == 'A')
				return "\xB2\xE2\xBE\xE0";                       /* 测距 */
			return en;
		case 'H': return "\xCC\xFD\xB5\xBD";                       /* 听到 HEARD */
		case 'W': return "\xB5\xC8\xB4\xFD";                       /* 等待 WAIT */
		case 'F':
			if (en[1] == ':') return "\xC9\xBE\xB3\xFD";         /* 删除 F:DEL */
			if (en[1] == 'O') return "\xB7\xA2\xCF\xD6\x3A";     /* 发现: */
			return en;
		case 'N':
			if (en[1] == 'O' && en[2] == 'T')
				return "\xCE\xB4\xB7\xA2\xCF\xD6";               /* 未发现 */
			if (en[1] == 'O' && en[2] == ' ')
				return "\xCE\xDE\xBC\xC7\xC2\xBC";               /* 无记录 */
			return en;
		case 'L':
			if (en[1] == 'A') return "\xCC\xFD\xB5\xBD\x3A";     /* 听到: */
			return en;
		default:  return en;
	}
}

static unsigned msg_text_width8(const char *s)
{
	unsigned w = 0;
	const size_t len = strlen(s);
	for (size_t i = 0; i < len; ) {
		const uint8_t c = (uint8_t)s[i];
		if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len &&
		    (uint8_t)s[i + 1] >= 0xA1 && (uint8_t)s[i + 1] <= 0xFE) {
			w += FONT8_SIZE;
			i += 2;
		} else {
			w += 7u;
			i++;
		}
	}
	return w;
}

/** Max byte count of s that fits within max_w pixels.
 *  ASCII=7px, GB2312=8px.  Stops at character boundary. */
static uint8_t msg_fit_bytes(const char *s, uint8_t max_w)
{
	unsigned w = 0;
	size_t i = 0;
	while (s[i]) {
		const bool hz = (uint8_t)s[i] >= 0xA1u && s[i + 1u];
		const unsigned cw = hz ? FONT8_SIZE : 7u;
		if (w + cw > max_w) break;
		w += cw;
		i += 1u + hz;
	}
	return (uint8_t)i;
}


static void format_age(uint16_t seconds, char *buf, uint8_t len)
{
    if (!buf || len == 0u) return;
    if (seconds < 60u) snprintf(buf, len, "NOW");
    else if (seconds < 3600u) snprintf(buf, len, "%um", (unsigned)(seconds / 60u));
    else snprintf(buf, len, "%uh", (unsigned)(seconds / 3600u));
}

static const char *packet_type_short(uint8_t type)
{
    switch (type) {
        case MSG_PKT_TYPE_TEXT: return "MSG";
        case MSG_PKT_TYPE_ACK:  return "ACK";
        case MSG_PKT_TYPE_PING: return "PNG";
        case MSG_PKT_TYPE_PONG: return "PON";
        default: return "---";
    }
}

static void print_line(const char *s, uint8_t line, bool sel)
{
    /* Render the text first, then invert the whole row when selected.
     * UI_PrintStringSmallNormalInverse sizes its invert box with strlen(),
     * which double-counts 2-byte GB2312 chars (e.g. 14 hanzi = 28 bytes
     * → invert width 28*7=196 > 128), overflowing past x=127 into the
     * next display row.  Full-row invert matches draw_home's capsule style
     * and never overflows. */
    char safe[40];
    strncpy(safe, s, sizeof(safe) - 1U);
    safe[sizeof(safe) - 1U] = 0;

    UI_PrintStringSmallNormal(safe, 1, 0, line);
    if (sel) {
        for (uint8_t x = 0; x < 128; x++)
            gFrameBuffer[line][x] ^= 0xFF;
    }
}

static void print_right_small(const char *s, uint8_t line)
{
    // SmallNormal pitch is 7 px.  Keep a wider right margin because the real
    // LCD showed the final digit wrapping when drawn too close to x=127.
    uint8_t len = (uint8_t)strlen(s);
    uint8_t width = (uint8_t)(len * 7U);
    uint8_t x = (width >= 120U) ? 0 : (uint8_t)(122U - width);
    UI_PrintStringSmallNormal(s, x, 0, line);
}

static void draw_title(const char *s)
{
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    const char *t = msg_label(s);

    /* Home title「信息」: 16×16 centered, up 2px into status band (screen y=6). */
    if (msg_lang_cn() && s != NULL && strcmp(s, "MESSENGER") == 0) {
        const size_t len = strlen(t);
        unsigned units = 0;
        size_t i;
        uint8_t cur;

        for (i = 0; i < len; ) {
            const uint8_t c = (uint8_t)t[i];
            if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len &&
                (uint8_t)t[i + 1] >= 0xA1 && (uint8_t)t[i + 1] <= 0xFE) {
                units += 2u;
                i += 2;
            } else {
                units += 1u;
                i++;
            }
        }
        cur = (uint8_t)((128u - (units * 8u)) / 2u);

        for (i = 0; i < len; ) {
            const uint8_t c = (uint8_t)t[i];
            if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len) {
                const uint8_t lo = (uint8_t)t[i + 1];
                if (lo >= 0xA1 && lo <= 0xFE) {
                    const uint32_t idx = (uint32_t)(c - 0xA1) * 94u + (lo - 0xA1);
                    uint8_t cnDot[FONT16_SIZE];
                    uint8_t col;
                    PY25Q16_ReadBuffer(FLASH_FONT16_BASE + idx * FONT16_SIZE, cnDot, FONT16_SIZE);
                    // 裁剪左右空白:只渲染中间的11列
                    for (col = CN_LEFT_MARGIN; col < CN_LEFT_MARGIN + CN_WIDTH; col++) {
                        const uint8_t bits_lo = cnDot[col];
                        const uint8_t bits_hi = cnDot[col + 16u];
                        uint8_t row;
                        for (row = 0; row < 16u; row++) {
                            const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
                            const uint8_t bit = (uint8_t)(row & 7u);
                            if (bits & (uint8_t)(1u << bit)) {
                                const uint8_t ay = (uint8_t)(6u + row); /* up 2px vs fb-only title */
                                const uint8_t px = (uint8_t)(cur + (col - CN_LEFT_MARGIN));
                                if (ay < 8u)
                                    PutPixelStatus(px, ay, true);
                                else
                                    PutPixel(px, (uint8_t)(ay - 8u), true);
                            }
                        }
                    }
                    cur = (uint8_t)(cur + CN_WIDTH);
                    i += 2;
                    continue;
                }
            }
            i++;
        }
        return;
    }

    /* Other titles (INBOX/COMPOSE/SENT/DRAFTS): 8×8 when Chinese */
    if (msg_lang_cn() && UI_StringHasGb2312(t)) {
        const unsigned w = msg_text_width8(t);
        const uint8_t x = (w >= 128u) ? 0u : (uint8_t)((128u - w) / 2u);
        UI_PrintStringSmallBold(t, x, 0, 0);
        return;
    }

    {
        uint8_t len = (uint8_t)strlen(t);
        /* SmallBold is visually closer to a 7 px pitch on the UV-K1 LCD. */
        uint8_t x = (len >= 18) ? 0 : (uint8_t)((128U - (len * 7U)) / 2U);
        UI_PrintStringSmallBold(t, x, 0, 0);
    }
}

static void draw_dotted_separator(uint8_t y)
{
    /* Light message separator: one filled segment, one gap.
     * Direct framebuffer write avoids 32 calls to UI_DrawLineBuffer. */
    const uint8_t mask = (uint8_t)(1u << (y & 7u));
    uint8_t *row = gFrameBuffer[y >> 3];
    for (uint8_t x = 0; x < 128u; x += 4u) {
        row[x] |= mask;
        row[(uint8_t)(x + 1u)] |= mask;
    }
}

static void msg_set_pixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= 128U || y >= 64U) return;
    uint8_t mask = (uint8_t)(1U << (y & 7U));
    if (on) gFrameBuffer[y >> 3][x] |= mask;
    else    gFrameBuffer[y >> 3][x] &= (uint8_t)~mask;
}

static void msg_fill_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on)
{
    if (x1 > 127U) x1 = 127U;
    if (y1 > 63U) y1 = 63U;
    for (uint8_t y = y0; y <= y1; y++) {
        for (uint8_t x = x0; x <= x1; x++) msg_set_pixel(x, y, on);
    }
}

static uint8_t range_rssi_bars(int8_t rssi)
{
    /* Same visual scale as the UV-K5 GOGUFW Messenger UI. */
    if (rssi > -75)  return 5u;
    if (rssi > -88)  return 4u;
    if (rssi > -100) return 3u;
    if (rssi > -112) return 2u;
    if (rssi > -124) return 1u;
    return 0u;
}

static void draw_rssi_bars(uint8_t x, uint8_t y, int8_t rssi)
{
    uint8_t bars = range_rssi_bars(rssi);
    if (bars > 5u) bars = 5u;

    /* UV-K5-style five-step bar: inactive bars remain visible as a small
     * baseline tick, active bars are filled vertical columns. */
    for (uint8_t i = 0u; i < 5u; i++) {
        const uint8_t h = (uint8_t)(2u + i);
        const uint8_t bx = (uint8_t)(x + i * 4u);
        const uint8_t base = (uint8_t)(y + 6u);
        const uint8_t y0 = (uint8_t)(base - h);
        if (i < bars) msg_fill_rect(bx, y0, (uint8_t)(bx + 2u), base, true);
        else UI_DrawLineBuffer(gFrameBuffer, bx, base, (uint8_t)(bx + 2u), base, 1);
    }
}

static void __attribute__((unused)) draw_rssi_bars_compact(uint8_t x, uint8_t y, int8_t rssi)
{
    uint8_t bars = range_rssi_bars(rssi);
    if (bars > 5u) bars = 5u;

    /* Compact variant for Range Check rows.  The text row must fit:
     * ID(6 chars) + RSSI(4 chars) + 5-bar meter + voltage(4 chars).
     * Keep the five UV-K5-like baseline ticks, but use a 3 px pitch so the
     * bar block ends before the right-aligned voltage column. */
    for (uint8_t i = 0u; i < 5u; i++) {
        const uint8_t h = (uint8_t)(2u + i);
        const uint8_t bx = (uint8_t)(x + i * 3u);
        const uint8_t base = (uint8_t)(y + 6u);
        const uint8_t y0 = (uint8_t)(base - h);
        if (i < bars) msg_fill_rect(bx, y0, (uint8_t)(bx + 1u), base, true);
        else UI_DrawLineBuffer(gFrameBuffer, bx, base, (uint8_t)(bx + 1u), base, 1);
    }
}

static void msg_draw_small_at_y(const char *s, uint8_t x, uint8_t y, bool inverted)
{
    const uint8_t pitch = 7U;
    uint8_t len = (uint8_t)strlen(s);
    if (inverted) {
        uint8_t w = (uint8_t)(len * pitch + 2U);
        msg_fill_rect(x ? (uint8_t)(x - 1U) : 0U, y ? (uint8_t)(y - 1U) : 0U,
                      (uint8_t)(x + w), (uint8_t)(y + 7U), true);
    }

    for (uint8_t i = 0; s[i] && x < 128U; i++, x = (uint8_t)(x + pitch)) {
        char c = s[i];
        if (c <= ' ' || c >= 127) continue;
        const uint8_t *glyph = gFontSmall[(uint8_t)c - ' ' - 1U];
        for (uint8_t col = 0; col < 6U; col++) {
            uint8_t bits = glyph[col];
            for (uint8_t row = 0; row < 7U; row++) {
                if (bits & (1U << row)) msg_set_pixel((uint8_t)(x + col), (uint8_t)(y + row), !inverted);
            }
        }
    }
}

/** 16×16 GB2312 glyph at pixel Y (one character = 2 bytes). */
static void msg_draw_cn16_char_at_y(const char hz[2], uint8_t x, uint8_t y)
{
	const uint8_t c = (uint8_t)hz[0];
	const uint8_t lo = (uint8_t)hz[1];
	uint8_t cnDot[FONT16_SIZE];
	uint8_t col;

	if (c < 0xA1u || c > 0xF7u || lo < 0xA1u || lo > 0xFEu)
		return;
	PY25Q16_ReadBuffer(FLASH_FONT16_BASE + ((uint32_t)(c - 0xA1u) * 94u + (lo - 0xA1u)) * FONT16_SIZE,
	                   cnDot, FONT16_SIZE);
	// 裁剪左右空白:只渲染中间的11列
	for (col = CN_LEFT_MARGIN; col < CN_LEFT_MARGIN + CN_WIDTH && (uint8_t)(x + (col - CN_LEFT_MARGIN)) < 128u; col++) {
		const uint8_t bits_lo = cnDot[col];
		const uint8_t bits_hi = cnDot[col + 16u];
		uint8_t row;
		for (row = 0; row < 16u; row++) {
			const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
			if (bits & (uint8_t)(1u << (row & 7u)))
				msg_set_pixel((uint8_t)(x + (col - CN_LEFT_MARGIN)), (uint8_t)(y + row), true);
		}
	}
}

/** Digit (small) + 16×16 hanzi, evenly spaced with gaps in [x0,x1]. */
static void msg_draw_py_cands16(uint8_t x0, uint8_t x1, uint8_t y, const PY_CandView_t *cand)
{
	const uint8_t dig_w = 6u;
	const uint8_t hz_w = 16u;
	uint8_t n, span, i, gap, extra, x, rem;

	if (!cand || cand->count == 0u)
		return;
	n = cand->count;
	if (n > 7u)
		n = 7u;
	span = (uint8_t)(x1 - x0 + 1u);
	rem = (span > (uint8_t)(n * hz_w)) ? (uint8_t)(span - n * hz_w) : 0u;
	gap = (uint8_t)(rem / (n + 1u));
	extra = (uint8_t)(rem % (n + 1u));
	x = (uint8_t)(x0 + gap);
	for (i = 0; i < n; i++) {
		char dig[2];
		char hz[2];
		dig[0] = (char)('1' + i);
		dig[1] = '\0';
		hz[0] = cand->hanzi[i * 2u];
		hz[1] = cand->hanzi[i * 2u + 1u];
		msg_draw_cn16_char_at_y(hz, x, y);
		msg_draw_small_at_y(dig, (x > x0 + dig_w) ? (uint8_t)(x - dig_w) : x0,
		                    (uint8_t)(y + 4u), false);
		x = (uint8_t)(x + hz_w + gap + (i < extra ? 1u : 0u));
	}
}

/** Pinyin syllables left-aligned with 1px gap between items. */
static void msg_draw_py_sylls(uint8_t x0, uint8_t x1, uint8_t y, const PY_CandView_t *cand)
{
	uint8_t n, i, x;

	if (!cand || cand->syll_n == 0u)
		return;
	n = cand->syll_n;
	if (n > PY_SYLL_MAX)
		n = PY_SYLL_MAX;
	x = x0;
	for (i = 0; i < n && x <= x1; i++) {
		char buf[10];
		uint8_t p = 0;
		uint8_t w;
		if (i == cand->syll_sel)
			buf[p++] = '>';
		strncpy(buf + p, cand->syll[i], sizeof(buf) - p - 1u);
		buf[sizeof(buf) - 1u] = '\0';
		msg_draw_small_at_y(buf, x, y, false);
		w = (uint8_t)(strlen(buf) * 7u);
		x = (uint8_t)(x + w + 1u); /* 1px gap between syllables */
	}
}

/** 8×8 GB2312 (and ASCII) at arbitrary framebuffer pixel Y — for CN home menu. */
static void msg_draw_cn8_at_y(const char *s, uint8_t x, uint8_t y, bool inverted)
{
	const unsigned w = msg_text_width8(s);
	uint8_t cur = x;
	const size_t len = strlen(s);

	if (inverted) {
		msg_fill_rect(x ? (uint8_t)(x - 1U) : 0U, y ? (uint8_t)(y - 1U) : 0U,
		              (uint8_t)(x + w), (uint8_t)(y + 7U), true);
	}

	for (size_t i = 0; i < len && cur < 128U; ) {
		const uint8_t c = (uint8_t)s[i];

		if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len) {
			const uint8_t lo = (uint8_t)s[i + 1];
			if (lo >= 0xA1 && lo <= 0xFE) {
				const uint32_t idx = (uint32_t)(c - 0xA1) * 94u + (lo - 0xA1);
				uint8_t cnDot[FONT8_SIZE];
				uint8_t col;
				PY25Q16_ReadBuffer(FLASH_FONT8_BASE + idx * FONT8_SIZE, cnDot, FONT8_SIZE);
				for (col = 0; col < FONT8_SIZE && (uint8_t)(cur + col) < 128U; col++) {
					const uint8_t bits = cnDot[col];
					uint8_t row;
					for (row = 0; row < 8u; row++) {
						if (bits & (uint8_t)(1u << row))
							msg_set_pixel((uint8_t)(cur + col), (uint8_t)(y + row), !inverted);
					}
				}
				cur = (uint8_t)(cur + FONT8_SIZE);
				i += 2;
				continue;
			}
		}

		if (c > ' ' && c < 127) {
			const uint8_t *glyph = gFontSmall[(uint8_t)c - ' ' - 1U];
			for (uint8_t col = 0; col < 6U && (uint8_t)(cur + col) < 128U; col++) {
				const uint8_t bits = glyph[col];
				for (uint8_t row = 0; row < 7U; row++) {
					if (bits & (uint8_t)(1U << row))
						msg_set_pixel((uint8_t)(cur + col), (uint8_t)(y + row), !inverted);
				}
			}
			cur = (uint8_t)(cur + 7U);
		}
		i++;
	}
}

/** Footer hint: 8×8 GB2312 when Chinese, else 3×5. right → align to x=128. */
static void msg_draw_hint(const char *en, uint8_t x, uint8_t y, bool right)
{
	const char *t = msg_label(en);
	const bool  cn = msg_lang_cn() && UI_StringHasGb2312(t);

	if (right) {
		const unsigned w = cn ? msg_text_width8(t) : (unsigned)(strlen(t) * 4u);
		x = (w >= 128u) ? 0u : (uint8_t)(128u - w);
	}
	if (cn)
		msg_draw_cn8_at_y(t, x, y, false);
	else
		GUI_DisplaySmallest(t, x, y, false, true);
}

/* Collect wrapped line starts. big16: CN=16/ASCII=8; else CN=8/ASCII=7. */
static uint8_t compose_collect_lines(const char *s, bool big16, uint8_t *starts, uint8_t max_starts)
{
	const size_t len = strlen(s);
	size_t i = 0;
	uint8_t nlines = 0;
	const unsigned ascii_w = big16 ? 8u : 7u;
	const unsigned cn_w = big16 ? CN_WIDTH : FONT8_SIZE;

	if (len == 0 || max_starts == 0)
		return 0;

	while (i < len && nlines < max_starts) {
		unsigned w = 0;
		size_t n = 0;

		starts[nlines++] = (uint8_t)i;
		while (i < len) {
			const uint8_t c = (uint8_t)s[i];
			unsigned cw;
			size_t step = 1;

			if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len &&
			    (uint8_t)s[i + 1] >= 0xA1 && (uint8_t)s[i + 1] <= 0xFE) {
				cw = cn_w;
				step = 2;
			} else {
				cw = ascii_w;
			}
			if (w + cw > 127u && n > 0)
				break;
			w += cw;
			i += step;
			n += step;
			if (n >= 39u)
				break;
		}
		if (n == 0) {
			n = ((uint8_t)s[i] >= 0xA1 && (i + 1) < len) ? 2u : 1u;
			i += n;
		}
	}
	return nlines;
}

/* Draw wrapped body.
 * tail: scroll is lines-from-end (compose follow-newest).
 * !tail: scroll is first visible line (read from start). */
static void print_wrapped_compose(const char *s, uint8_t y, uint8_t max_lines,
                                  bool big16, uint8_t scroll, bool tail)
{
	uint8_t starts[8];
	uint8_t nlines;
	uint8_t first;
	uint8_t row;
	char linebuf[40];
	const size_t len = strlen(s);

	if (max_lines == 0 || len == 0)
		return;

	nlines = compose_collect_lines(s, big16, starts, (uint8_t)ARRAY_SIZE(starts));
	if (nlines == 0)
		return;

	{
		uint8_t max_scroll = (nlines > max_lines) ? (uint8_t)(nlines - max_lines) : 0u;
		if (scroll > max_scroll)
			scroll = max_scroll;
		if (tail)
			first = (nlines > max_lines) ? (uint8_t)(nlines - max_lines - scroll) : 0u;
		else
			first = scroll;
	}

	for (row = 0; first + row < nlines && row < max_lines; row++) {
		size_t start = starts[first + row];
		size_t end = (first + row + 1u < nlines) ? starts[first + row + 1u] : len;
		size_t n = end - start;
		if (n >= sizeof(linebuf))
			n = sizeof(linebuf) - 1u;
		memcpy(linebuf, s + start, n);
		linebuf[n] = 0;
		if (big16)
			UI_PrintString(linebuf, 0, 0, (uint8_t)((y >> 3) + row * 2u), 8);
		else
			msg_draw_cn8_at_y(linebuf, 0, (uint8_t)(y + row * 8u), false);
	}
}

uint8_t MSG_UI_TextLineCount16(const char *s)
{
	uint8_t starts[8];
	if (!s)
		return 0;
	return compose_collect_lines(s, true, starts, (uint8_t)ARRAY_SIZE(starts));
}

static void msg_draw_hline(uint8_t x0, uint8_t x1, uint8_t y, bool on)
{
    if (x1 > 127U) x1 = 127U;
    for (uint8_t x = x0; x <= x1; x++) msg_set_pixel(x, y, on);
}

static void msg_draw_vline(uint8_t x, uint8_t y0, uint8_t y1, bool on)
{
    if (y1 > 63U) y1 = 63U;
    for (uint8_t y = y0; y <= y1; y++) msg_set_pixel(x, y, on);
}

static void msg_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on)
{
    int dx = (x1 > x0) ? (int)(x1 - x0) : (int)(x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -((y1 > y0) ? (int)(y1 - y0) : (int)(y0 - y1));
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int x = x0, y = y0;
    while (1) {
        msg_set_pixel((uint8_t)x, (uint8_t)y, on);
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

static void draw_icon_envelope(uint8_t x, uint8_t y)
{
    /* HOME icon: clean 30x18 envelope, centered in the right-side icon area. */
    msg_draw_hline(x, (uint8_t)(x + 29U), y, true);
    msg_draw_hline(x, (uint8_t)(x + 29U), (uint8_t)(y + 17U), true);
    msg_draw_vline(x, y, (uint8_t)(y + 17U), true);
    msg_draw_vline((uint8_t)(x + 29U), y, (uint8_t)(y + 17U), true);
    msg_draw_line((uint8_t)(x + 1U), (uint8_t)(y + 2U), (uint8_t)(x + 14U), (uint8_t)(y + 10U), true);
    msg_draw_line((uint8_t)(x + 28U), (uint8_t)(y + 2U), (uint8_t)(x + 15U), (uint8_t)(y + 10U), true);
    msg_draw_line((uint8_t)(x + 1U), (uint8_t)(y + 16U), (uint8_t)(x + 11U), (uint8_t)(y + 9U), true);
    msg_draw_line((uint8_t)(x + 28U), (uint8_t)(y + 16U), (uint8_t)(x + 18U), (uint8_t)(y + 9U), true);
}

static void draw_icon_pencil(uint8_t x, uint8_t y)
{
    /* HOME icon: clear diagonal pencil, 30x28.  Drawn as a thick slanted
     * body with a visible point and a short baseline, matching the UI mockup. */
    for (uint8_t i = 0; i < 20U; i++) {
        uint8_t px = (uint8_t)(x + 4U + i);
        uint8_t py = (uint8_t)(y + 23U - i);
        msg_set_pixel(px, py, true);
        msg_set_pixel((uint8_t)(px + 1U), py, true);
        msg_set_pixel(px, (uint8_t)(py - 1U), true);
        msg_set_pixel((uint8_t)(px + 1U), (uint8_t)(py - 1U), true);
    }
    /* pencil tip */
    msg_draw_line((uint8_t)(x + 2U), (uint8_t)(y + 25U), (uint8_t)(x + 6U), (uint8_t)(y + 23U), true);
    msg_draw_line((uint8_t)(x + 2U), (uint8_t)(y + 25U), (uint8_t)(x + 4U), (uint8_t)(y + 21U), true);
    msg_set_pixel((uint8_t)(x + 1U), (uint8_t)(y + 26U), true);
    /* eraser/cap */
    msg_draw_line((uint8_t)(x + 22U), (uint8_t)(y + 3U), (uint8_t)(x + 26U), y, true);
    msg_draw_line((uint8_t)(x + 24U), (uint8_t)(y + 6U), (uint8_t)(x + 29U), (uint8_t)(y + 2U), true);
    msg_draw_line((uint8_t)(x + 26U), y, (uint8_t)(x + 29U), (uint8_t)(y + 2U), true);
    msg_draw_line((uint8_t)(x + 22U), (uint8_t)(y + 3U), (uint8_t)(x + 24U), (uint8_t)(y + 6U), true);
    /* writing line */
    msg_draw_hline((uint8_t)(x + 10U), (uint8_t)(x + 29U), (uint8_t)(y + 27U), true);
}

static void draw_icon_up_arrow(uint8_t x, uint8_t y)
{
    /* HOME icon: bold upload/up arrow, 31x27. */
    const uint8_t cx = (uint8_t)(x + 15U);
    for (uint8_t r = 0; r < 9U; r++) {
        msg_draw_hline((uint8_t)(cx - r), (uint8_t)(cx + r), (uint8_t)(y + r), true);
    }
    msg_fill_rect((uint8_t)(x + 12U), (uint8_t)(y + 9U), (uint8_t)(x + 18U), (uint8_t)(y + 22U), true);
    msg_draw_hline((uint8_t)(x + 4U), (uint8_t)(x + 26U), (uint8_t)(y + 26U), true);
}

static void draw_icon_floppy(uint8_t x, uint8_t y)
{
    /* HOME icon: visually square floppy disk.
     * The LCD pixels look slightly taller than wide, so the bitmap is
     * intentionally wider than high (34x24) to read as a square on-device.
     * Outer shape: square body with only the top-right corner cut off. */
    msg_draw_hline(x, (uint8_t)(x + 28U), y, true);                 /* top edge stops at the cut */
    msg_draw_line((uint8_t)(x + 29U), y, (uint8_t)(x + 33U), (uint8_t)(y + 4U), true);
    msg_draw_vline((uint8_t)(x + 33U), (uint8_t)(y + 4U), (uint8_t)(y + 23U), true);
    msg_draw_hline(x, (uint8_t)(x + 33U), (uint8_t)(y + 23U), true);
    msg_draw_vline(x, y, (uint8_t)(y + 23U), true);

    /* Top shutter/label area. */
    msg_draw_hline((uint8_t)(x + 4U), (uint8_t)(x + 23U), (uint8_t)(y + 3U), true);
    msg_draw_hline((uint8_t)(x + 4U), (uint8_t)(x + 23U), (uint8_t)(y + 9U), true);
    msg_draw_vline((uint8_t)(x + 4U), (uint8_t)(y + 3U), (uint8_t)(y + 9U), true);
    msg_draw_vline((uint8_t)(x + 23U), (uint8_t)(y + 3U), (uint8_t)(y + 9U), true);
    msg_fill_rect((uint8_t)(x + 17U), (uint8_t)(y + 4U), (uint8_t)(x + 20U), (uint8_t)(y + 7U), true);

    /* Bottom label window, kept wide to avoid a tall/narrow look. */
    msg_draw_hline((uint8_t)(x + 6U), (uint8_t)(x + 27U), (uint8_t)(y + 14U), true);
    msg_draw_hline((uint8_t)(x + 6U), (uint8_t)(x + 27U), (uint8_t)(y + 22U), true);
    msg_draw_vline((uint8_t)(x + 6U), (uint8_t)(y + 14U), (uint8_t)(y + 22U), true);
    msg_draw_vline((uint8_t)(x + 27U), (uint8_t)(y + 14U), (uint8_t)(y + 22U), true);
    msg_draw_hline((uint8_t)(x + 9U), (uint8_t)(x + 24U), (uint8_t)(y + 17U), true);
    msg_draw_hline((uint8_t)(x + 9U), (uint8_t)(x + 24U), (uint8_t)(y + 19U), true);
}

static void draw_home_icon(uint8_t idx, uint8_t cy)
{
    /* Icons centered around cy in the right side.
     * cy is the vertical center of the whole menu area (not tied to selection). */
    switch (idx) {
        case 0: draw_icon_envelope(88U, (uint8_t)(cy - 9U)); break;  /* 30x18 */
        case 1: draw_icon_pencil(89U, (uint8_t)(cy - 14U)); break;   /* 30x28 */
        case 2: draw_icon_up_arrow(88U, (uint8_t)(cy - 13U)); break; /* 31x27 */
        default: draw_icon_floppy(86U, (uint8_t)(cy - 12U)); break;  /* 34x24 */
    }
}


static void draw_home(void)
{
    static const char *items[] = { "INBOX", "COMPOSE", "SENT", "DRAFTS" };
    const uint8_t count = 4u;
    const uint8_t sel  = gMsgHomeCursor;
    const bool    cn   = msg_lang_cn();

    /* Layout: only 2 rows visible (selected 16px + 1 neighbour 8px).
     * Selected = 2+16+2 = 20px, neighbour = 8+1 = 9px, total ≤ 29px. */
    const uint8_t FIRST_Y    = cn ? 14u : 10u;
    const uint8_t CLIP_Y     = cn ? 46u : 45u; /* matches bottom sep (up 1px) */
    const uint8_t SEL_PAD    = 2u;
    const uint8_t SEL_NEED   = (uint8_t)(SEL_PAD + 16u + SEL_PAD);   /* 20 */
    const uint8_t NORM_PITCH = 9u;

    draw_title("MESSENGER");

    /* --- Only show selected row and one adjacent row (2 rows total) --- */
    uint8_t first = sel;
    /* Prefer showing one item above the selection when possible */
    if (sel > 0u) first = (uint8_t)(sel - 1u);

    /* --- Draw visible items (max 2 rows) --- */
    uint8_t y_cursor = FIRST_Y;
    uint8_t drawn = 0u;

    for (uint8_t i = first; i < count && drawn < 2u; i++) {
        const bool    is_sel = (i == sel);
        const uint8_t y0     = y_cursor;
        uint8_t       need;

        if (y0 >= CLIP_Y) break;

        need = is_sel ? SEL_NEED : NORM_PITCH;
        {
            const uint8_t avail = (uint8_t)(CLIP_Y - y0);
            if (need > avail) need = avail;
        }

        const char *label = msg_label(items[i]);

        if (is_sel) {
            const uint8_t text_y    = (uint8_t)(y0 + SEL_PAD);
            uint8_t       text_rows = (uint8_t)(need > 2u * SEL_PAD)
                                     ? (uint8_t)(need - 2u * SEL_PAD) : 0u;
            if (text_rows > 16u) text_rows = 16u;

            /* Draw 16×16 text normally, then invert the left band */
            UI_MENU_PrintBigAtY(label, 2u, 0u, text_y, text_rows);
            for (uint8_t yy = y0; yy <= (uint8_t)(y0 + need - 1u) && yy < 64u; yy++) {
                const uint8_t mask = (uint8_t)(1u << (yy & 7u));
                for (uint8_t xx = 0u; xx < 85u; xx++)
                    gFrameBuffer[yy >> 3][xx] ^= mask;
            }
            y_cursor = (uint8_t)(y0 + SEL_NEED + 1u);
        } else {
            if (cn && UI_StringHasGb2312(label))
                msg_draw_cn8_at_y(label, 1u, y0, false);
            else
                msg_draw_small_at_y(label, 1u, y0, false);
            y_cursor = (uint8_t)(y0 + NORM_PITCH);
        }
        drawn++;
    }

    /* Icon: vertically centered in the menu area (FIRST_Y..CLIP_Y), not tied to selection */
    {
        const uint8_t menu_cy = (uint8_t)((FIRST_Y + CLIP_Y) / 2u);
        draw_home_icon(sel, menu_cy);
    }

    {
        const uint8_t sep_y = cn ? 46U : 45U; /* up 1px */
        const uint8_t sel_y = cn ? 48U : 47U; /* up 2px */
        draw_dotted_separator(sep_y);
        msg_draw_hint("SELECT", 0, sel_y, false);
    }
}

static MSG_Message_t *current_list(uint8_t *count, const char **title)
{
    if (gMsgScreen == MSG_SCREEN_INBOX) { *count = MSG_STORE_CountInbox(); *title = "INBOX"; return gMessengerInbox; }
    if (gMsgScreen == MSG_SCREEN_OUTBOX) { *count = MSG_STORE_CountOutbox(); *title = "SENT"; return gMessengerOutbox; }
    *count = MSG_DRAFT_CAPACITY;
    /* gMsgDraftMode reuses the drafts list as either a "save into slot"
     * picker (1) or a "delete this slot" confirm (2); both swap the title.
     * Passing a GB2312 string directly makes draw_title render it as CN. */
    *title = (gMsgDraftMode == 1u) ? (msg_lang_cn() ? "\xCA\xC7\xB7\xF1\xB1\xA3\xB4\xE6" : "SAVE?")
           : (gMsgDraftMode == 2u) ? (msg_lang_cn() ? "\xCA\xC7\xB7\xF1\xC9\xBE\xB3\xFD" : "DELETE?")
           : "DRAFTS";
    return 0;
}

static void draw_list(void)
{
    uint8_t count; const char *title;
    MSG_Message_t *list = current_list(&count, &title);
    char buf[40];
    draw_title(title);
    snprintf(buf, sizeof(buf), "%u/%u", count ? (gMsgCursor + 1) : 0, count);
    print_right_small(buf, 0);
    if (!count) { UI_PrintStringSmallNormal(msg_label("EMPTY"), 0, 0, 3); return; }
    for (uint8_t row = 0; row < 6; row++) {
        uint8_t idx = gMsgScroll + row;
        if (idx >= count) break;
        if (gMsgScreen == MSG_SCREEN_DRAFTS) {
            const char *d = MSG_STORE_GetDraft(idx);
            /* "N " prefix = 14px; available for text: 127-14-1 = 112px */
            uint8_t n = msg_fit_bytes(d, 112u);
            snprintf(buf, sizeof(buf), "%u %.*s", idx + 1, n, d);
        }
        else {
            /* INBOX and OUTBOX share the same "%c%-11.11s%4s" row layout;
             * only the leading status char differs. */
            char st;
            char age[5];
            format_age(list[idx].age_seconds, age, sizeof(age));
            if (gMsgScreen == MSG_SCREEN_OUTBOX)
                st = (list[idx].status == MSG_STATUS_ACKED) ? '+'
                   : (list[idx].status == MSG_STATUS_FAILED) ? 'x' : '?';
            else
                st = list[idx].unread ? '*' : ' ';
            snprintf(buf, sizeof(buf), "%c%-11.11s%4s", st, list[idx].text, age);
        }
        print_line(buf, row + 1, idx == gMsgCursor);
    }
}

static void draw_read(void)
{
    MSG_Message_t *m = (gMsgReadSource == MSG_SCREEN_OUTBOX) ? &gMessengerOutbox[gMsgReadIndex] : &gMessengerInbox[gMsgReadIndex];
    char buf[32];
    uint8_t body_lines;
    uint8_t starts[8];

    draw_title((gMsgReadSource == MSG_SCREEN_OUTBOX) ? "SENT" : "READ");
    {
        uint8_t total = (gMsgReadSource == MSG_SCREEN_OUTBOX) ? MSG_STORE_CountOutbox() : MSG_STORE_CountInbox();
        snprintf(buf, sizeof(buf), "%u/%u", total ? (uint8_t)(gMsgReadIndex + 1U) : 0U, total);
        print_right_small(buf, 0);
    }

    uint8_t used_hops = (m->ttl_init >= m->ttl_remain) ? (uint8_t)(m->ttl_init - m->ttl_remain) : 0;
    char age[5];
    format_age(m->age_seconds, age, sizeof(age));
    if (gMsgReadSource == MSG_SCREEN_OUTBOX) {
        char st = '?';
        if (m->status == MSG_STATUS_ACKED) st = '+';
        else if (m->status == MSG_STATUS_FAILED) st = 'x';

        if (gMessengerConfig.msg_hop == 0U) snprintf(buf, sizeof(buf), "TO:%s HOP:OFF %s", m->to, age);
        else snprintf(buf, sizeof(buf), "TO:%s HOP:%u %s", m->to, gMessengerConfig.msg_hop, age);

        GUI_DisplaySmallest(buf, 0, 9, false, true);
        char stbuf[2] = { st, 0 };
        UI_PrintStringSmallBold(stbuf, 120, 0, 1);
    } else {
        if (m->ttl_init == 0U) snprintf(buf, sizeof(buf), "FROM:%s HOP:OFF %s", m->from, age);
        else snprintf(buf, sizeof(buf), "FROM:%s HOP:%u/%u %s", m->from, used_hops, m->ttl_init, age);
        GUI_DisplaySmallest(buf, 0, 9, false, true);
    }

    draw_dotted_separator(15);

    /* 16×16 body: 2 rows (y=16..47). ACK row steals one body line. */
    body_lines = (gMsgReadSource == MSG_SCREEN_OUTBOX && m->ack_count > 0u) ? 1u : 2u;
    {
        uint8_t nlines = compose_collect_lines(m->text, true, starts, (uint8_t)ARRAY_SIZE(starts));
        uint8_t max_scroll = (nlines > body_lines) ? (uint8_t)(nlines - body_lines) : 0u;
        if (gMsgReadScroll > max_scroll)
            gMsgReadScroll = max_scroll;
    }
    print_wrapped_compose(m->text, 16, body_lines, true, gMsgReadScroll, false);

    if (gMsgReadSource == MSG_SCREEN_OUTBOX && m->ack_count > 0u) {
        char ackbuf[32];
        uint8_t pos = 0u;
        pos += (uint8_t)snprintf(ackbuf + pos, sizeof(ackbuf) - pos, "ACK:");
        for (uint8_t i = 0u; i < m->ack_count && i < MSG_ACK_SOURCE_MAX && pos < sizeof(ackbuf); i++) {
            pos += (uint8_t)snprintf(ackbuf + pos, sizeof(ackbuf) - pos, "%s%.*s",
                                     i ? " " : "", MSG_ACK_ID_LEN, m->ack_from[i]);
        }
        GUI_DisplaySmallest(ackbuf, 0, 36, false, true);
    }

    draw_dotted_separator(47); /* up 1px */
    if (gMsgReadSource == MSG_SCREEN_OUTBOX)
        msg_draw_hint("RESEND", 0, 48, false); /* up 2px */
    else
        msg_draw_hint("RE:", 0, 48, false);
    msg_draw_hint("F:DEL", 0, 48, true);
}

static void draw_compose(void)
{
    char buf[12];
    uint8_t body_lines;
    uint8_t body_y;
    uint8_t starts[8];
    PY_CandView_t cand;
    const bool py = (gMsgEditor.mode == 3U);
    const bool py_composing = py && PY_IsComposing();
    const bool big16 = !py_composing;

    draw_title("COMPOSE");

    snprintf(buf, sizeof(buf), "%u/%u", (uint8_t)strlen(gMsgComposeBuf), (uint8_t)MSG_TEXT_LEN);
    {
        uint8_t w = (uint8_t)(strlen(buf) * 4U);
        GUI_DisplaySmallest(buf, (w >= 128U) ? 0U : (uint8_t)(127U - w), 9, false, true);
    }

    memset(&cand, 0, sizeof(cand));
    if (py)
        PY_GetCandView(&cand);

    body_lines = (py_composing && cand.phase == 2u) ? 1u : 2u;
    /* Idle 16×16 uses UI_PrintString (page-aligned y=16 / line 2+4). */
    body_y = big16 ? 16u : 20u;
    draw_dotted_separator(big16 ? 15u : 17u);

    {
        uint8_t nlines = compose_collect_lines(gMsgComposeBuf, big16, starts, (uint8_t)ARRAY_SIZE(starts));
        uint8_t max_scroll = (nlines > body_lines) ? (uint8_t)(nlines - body_lines) : 0u;
        if (gMsgComposeScroll > max_scroll)
            gMsgComposeScroll = max_scroll;
    }

    print_wrapped_compose(gMsgComposeBuf, body_y, body_lines, big16, gMsgComposeScroll, true);

    if (py && cand.phase >= 1u) {
        if (cand.phase == 2u) {
            msg_draw_py_sylls(0, 127, 27, &cand);
            msg_draw_py_cands16(0, 127, 34, &cand);
            msg_draw_hint("SEND", 0, 49, false); /* up 2px */
            GUI_DisplaySmallest("pinyin", 92, 49, false, true);
            return;
        }
        msg_draw_py_sylls(0, 127, (uint8_t)(body_y + body_lines * 8u), &cand);
    }

    /* Below 2×16 body. */
    draw_dotted_separator(46); /* compose-only: bottom sep up 1px */
    msg_draw_hint("SEND", 0, 48, false);
    if (py)
        GUI_DisplaySmallest("pinyin", 92, 48, false, true);
    else
        GUI_DisplaySmallest((gMsgEditor.mode == 2U) ? "2" : (gMsgEditor.upper ? "B" : "b"), 120, 48, false, true);
}

static void draw_range(void)
{
    char buf[28];
    draw_title((gMsgRangeStatus == 1u || gMsgRangeStatus == 2u) ? "RANGE CHECK" : "HEARD");
    if (gMsgRangeStatus == 1u) msg_draw_hint("WAIT", 0, 1, false);
    else if (gMsgRangeStatus == 2u) msg_draw_hint("RESULT", 0, 1, false);

    const uint8_t top_sep = 9u;
    const uint8_t bottom_sep = 45u; /* up 1px */
    const uint8_t page_size = 3u;
    draw_dotted_separator(top_sep);
    draw_dotted_separator(bottom_sep);

    if (gMsgRangeStatus == 1u || gMsgRangeStatus == 2u) {
        /* Active Range Check result screen: show only PONG results from the
         * current ping session, live as they arrive, strongest RSSI first.
         * This is intentionally different from HEARD: no TYPE/AGE here. */
        uint8_t session_count = 0u;
        for (uint8_t i = 0; i < gMsgRangeCount; i++) {
            if (gMsgRangeFound[i].used && gMsgRangeFound[i].range_session == gMsgRangeSession) session_count++;
        }
        if (session_count == 0u) {
            if (gMsgRangeStatus == 1u) {
                if (msg_lang_cn()) {
                    msg_draw_cn8_at_y(msg_label("WAIT"), 48, 24, false);
                } else {
                    msg_draw_small_at_y("WAITING", 40, 20, false);
                    msg_draw_small_at_y("FOR PONG", 36, 29, false);
                }
            } else {
                if (msg_lang_cn())
                    msg_draw_cn8_at_y(msg_label("NOT FOUND"), 40, 24, false);
                else
                    msg_draw_small_at_y("NOT FOUND", 33, 24, false);
            }
        } else {
            msg_draw_hint("FOUND:", 0, 12, false);
            bool used[MSG_RANGE_MAX_FOUND];
            memset(used, 0, sizeof(used));
            uint8_t drawn = 0u;
            for (uint8_t row = 0; row < page_size; row++) {
                int8_t best_rssi = -128;
                uint8_t best = 0xFFu;
                for (uint8_t i = 0; i < gMsgRangeCount && i < MSG_RANGE_MAX_FOUND; i++) {
                    if (used[i]) continue;
                    if (!gMsgRangeFound[i].used || gMsgRangeFound[i].range_session != gMsgRangeSession) continue;
                    if (best == 0xFFu || gMsgRangeFound[i].rssi > best_rssi) {
                        best = i;
                        best_rssi = gMsgRangeFound[i].rssi;
                    }
                }
                if (best == 0xFFu) break;
                used[best] = true;
                const uint8_t y = (uint8_t)(19u + row * 9u);
                /* Fixed-column Range row layout.  IMPORTANT: the Range
                 * Check meter must be pixel-identical to the HEARD meter, so
                 * do not use a compact/thinner variant here.  Columns are
                 * shifted left enough to keep the full HEARD-width bar clear
                 * of the fixed voltage column by at least one small-font
                 * character width.
                 *
                 *   x=0..41   : 6-char callsign/msgid
                 *   x=43..70  : RSSI, right aligned to 4 chars (-102)
                 *   x=74..92  : HEARD-style 5-step RSSI bar, same pixels
                 *   x=93..99  : one small-font char gap before voltage
                 *   x=100..127: voltage, 4 chars (7.4V)
                 */
                snprintf(buf, sizeof(buf), "%-6s", gMsgRangeFound[best].callsign);
                msg_draw_small_at_y(buf, 0, y, false);
                snprintf(buf, sizeof(buf), "%4d", (int)gMsgRangeFound[best].rssi);
                msg_draw_small_at_y(buf, 43, y, false);
                draw_rssi_bars(74, y, gMsgRangeFound[best].rssi);
                snprintf(buf, sizeof(buf), "%u.%uV", (unsigned)(gMsgRangeFound[best].battery_cv / 100u), (unsigned)((gMsgRangeFound[best].battery_cv / 10u) % 10u));
                msg_draw_small_at_y(buf, 100, y, false);
                drawn++;
            }
            (void)drawn;
        }
    } else {
        msg_draw_hint("LAST HEARD:", 0, 12, false);
        uint8_t pages = 1u;
        if (gMsgRangeCount > 0u) pages = (uint8_t)((gMsgRangeCount + page_size - 1u) / page_size);
        if (gMsgRangeScroll >= pages) gMsgRangeScroll = (uint8_t)(pages - 1u);
        snprintf(buf, sizeof(buf), "%u/%u", (uint8_t)(gMsgRangeScroll + 1u), pages);
        uint8_t x = (uint8_t)(128u - (strlen(buf) * 4u));
        GUI_DisplaySmallest(buf, x, 12, false, true);

        if (gMsgRangeCount == 0u) {
            if (msg_lang_cn())
                msg_draw_cn8_at_y(msg_label("NO HEARD"), 40, 24, false);
            else
                msg_draw_small_at_y("NO HEARD", 36, 24, false);
        }
        for (uint8_t row = 0; row < page_size; row++) {
            uint8_t idx = (uint8_t)(gMsgRangeScroll * page_size + row);
            if (idx >= gMsgRangeCount) break;
            uint8_t y = (uint8_t)(19u + row * 9u);
            char age[5];
            format_age(gMsgRangeFound[idx].age_seconds, age, sizeof(age));
            /* Fixed-column HEARD row: use the full row while keeping
             * callsign/bar/type/age separated on the small LCD. */
            snprintf(buf, sizeof(buf), "%-6s", gMsgRangeFound[idx].callsign);
            msg_draw_small_at_y(buf, 0, y, false);
            draw_rssi_bars(48, y, gMsgRangeFound[idx].rssi);
            msg_draw_small_at_y(packet_type_short(gMsgRangeFound[idx].packet_type), 76, y, false);
            uint8_t age_x = (uint8_t)(127u - ((uint8_t)strlen(age) * 7u));
            msg_draw_small_at_y(age, age_x, y, false);
        }
    }

    GUI_DisplaySmallest("PING", 0, 47, false, true); /* up 2px */
    msg_draw_hint("EXIT", 0, 47, true);
}

void UI_DisplayMessenger(void)
{
    /* Do not clear/blit the status line here — that blanks battery/icons on
     * every key redraw (especially noticeable in pinyin compose). Status is
     * owned by UI_DisplayStatus via gUpdateStatus. */
    switch (gMsgScreen) {
        case MSG_SCREEN_HOME: draw_home(); break;
        case MSG_SCREEN_INBOX:
        case MSG_SCREEN_OUTBOX:
        case MSG_SCREEN_DRAFTS: draw_list(); break;
        case MSG_SCREEN_READ: draw_read(); break;
        case MSG_SCREEN_COMPOSE: draw_compose(); break;
        case MSG_SCREEN_RANGE: draw_range(); break;
        default: draw_home(); break;
    }
    ST7565_BlitFullScreen();
    gUpdateStatus = true;
}
