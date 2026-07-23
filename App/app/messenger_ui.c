#include <string.h>
#include <stdio.h>
#include "app/messenger_store.h"
#include "app/messenger_t9.h"
#include "app/messenger_rf.h"
#include "app/messenger_packet.h"
#include "app/pinyin_ime.h"
#include "driver/st7565.h"
#include "driver/py25q16.h"
#include "external/printf/printf.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/ui.h"
#include "misc.h"
#include "font.h"

#ifndef FLASH_FONT16_BASE
#define FLASH_FONT16_BASE  0x0A0000u
#define FLASH_FONT8_BASE   0x0E0000u
#define FONT16_SIZE        32u
#define FONT8_SIZE         8u
#endif

extern uint8_t gMsgHomeCursor;
extern uint8_t gMsgCursor;
extern uint8_t gMsgScroll;
extern uint8_t gMsgReadIndex;
extern uint8_t gMsgReadSource;
extern uint8_t gMsgSettingsCursor;
extern char gMsgComposeBuf[];
extern char gMsgCallsignBuf[];
extern MSG_T9Editor_t gMsgEditor;
extern MSG_T9Editor_t gMsgCallsignEditor;
extern uint8_t gMsgScreen;
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

enum { MSG_SCREEN_HOME = 0, MSG_SCREEN_INBOX, MSG_SCREEN_OUTBOX, MSG_SCREEN_DRAFTS, MSG_SCREEN_COMPOSE, MSG_SCREEN_READ, MSG_SCREEN_SETTINGS, MSG_SCREEN_CALLSIGN, MSG_SCREEN_RANGE };

static bool msg_lang_cn(void)
{
	return gUiLanguage == UI_LANGUAGE_CN;
}

/** English UI key → GB2312 label when language is Chinese. */
static const char *msg_label(const char *en)
{
	if (!msg_lang_cn() || en == NULL)
		return en;
	if (strcmp(en, "MESSENGER") == 0) return "\xD0\xC5\xCF\xA2";                 /* 信息 */
	if (strcmp(en, "INBOX") == 0)     return "\xCA\xD5\xBC\xFE\xCF\xE4";         /* 收件箱 */
	if (strcmp(en, "COMPOSE") == 0)   return "\xB1\xE0\xBC\xAD\xD0\xC5\xCF\xA2"; /* 编辑信息 */
	if (strcmp(en, "SENT") == 0)      return "\xB7\xA2\xBC\xFE\xCF\xE4";         /* 发件箱 */
	if (strcmp(en, "DRAFTS") == 0)    return "\xB2\xDD\xB8\xE5\xCF\xE4";         /* 草稿箱 */
	return en;
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
    // Keep all Messenger menu/list rows left-aligned and use the same
    // inverted selector style everywhere.  Important: do NOT pass a non-zero
    // End value to UI_PrintStringSmallNormalInverse here, because that helper
    // centers text when End > Start.  That was causing the text to drift
    // outside the selected capsule.
    char safe[18];
    strncpy(safe, s, sizeof(safe) - 1U);
    safe[sizeof(safe) - 1U] = 0;

    if (sel) UI_PrintStringSmallNormalInverse(safe, 1, 0, line);
    else UI_PrintStringSmallNormal(safe, 1, 0, line);
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

    /* Home title「信息」: 16×16 centered, up 4px into status band (screen y=4). */
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
                    for (col = 0; col < 16u; col++) {
                        const uint8_t bits_lo = cnDot[col];
                        const uint8_t bits_hi = cnDot[col + 16u];
                        uint8_t row;
                        for (row = 0; row < 16u; row++) {
                            const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
                            const uint8_t bit = (uint8_t)(row & 7u);
                            if (bits & (uint8_t)(1u << bit)) {
                                const uint8_t ay = (uint8_t)(4u + row); /* up 4px vs fb-only title */
                                const uint8_t px = (uint8_t)(cur + col);
                                if (ay < 8u)
                                    PutPixelStatus(px, ay, true);
                                else
                                    PutPixel(px, (uint8_t)(ay - 8u), true);
                            }
                        }
                    }
                    cur = (uint8_t)(cur + 16u);
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
    /* Light message separator: one filled segment, one gap.  It is less
     * visually heavy than a solid line and keeps the message text dominant. */
    for (uint8_t x = 0; x < 128U; x = (uint8_t)(x + 4U)) {
        UI_DrawLineBuffer(gFrameBuffer, x, y, (uint8_t)(x + 1U), y, 1);
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

static void print_line_y(const char *s, uint8_t y, bool sel)
{
    char safe[18];
    strncpy(safe, s, sizeof(safe) - 1U);
    safe[sizeof(safe) - 1U] = 0;
    msg_draw_small_at_y(safe, 1, y, sel);
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
	for (col = 0; col < 16u && (uint8_t)(x + col) < 128u; col++) {
		const uint8_t bits_lo = cnDot[col];
		const uint8_t bits_hi = cnDot[col + 16u];
		uint8_t row;
		for (row = 0; row < 16u; row++) {
			const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
			if (bits & (uint8_t)(1u << (row & 7u)))
				msg_set_pixel((uint8_t)(x + col), (uint8_t)(y + row), true);
		}
	}
}

/** Digit (small) + 16×16 hanzi, evenly spaced with gaps in [x0,x1]. */
static void msg_draw_py_cands16(uint8_t x0, uint8_t x1, uint8_t y, const PY_CandView_t *cand)
{
	const uint8_t dig_w = 6u;
	const uint8_t gap_dh = 2u; /* gap between digit and hanzi */
	const uint8_t hz_w = 16u;
	const uint8_t unit = (uint8_t)(dig_w + gap_dh + hz_w); /* 24 */
	uint8_t n, span, i, gap, extra, x;

	if (!cand || cand->count == 0u)
		return;
	n = cand->count;
	if (n > 7u)
		n = 7u;
	span = (uint8_t)(x1 - x0 + 1u);

	if ((uint16_t)n * unit + (n + 1u) <= span) {
		uint8_t rem = (uint8_t)(span - n * unit);
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
			/* Hanzi first, digit on top so "1" stays visible. */
			msg_draw_cn16_char_at_y(hz, (uint8_t)(x + dig_w + gap_dh), y);
			msg_draw_small_at_y(dig, x, (uint8_t)(y + 4u), false);
			x = (uint8_t)(x + unit + gap + (i < extra ? 1u : 0u));
		}
		return;
	}

	/* Narrow: even gaps between 16×16; digit painted after, to the left. */
	{
		uint8_t rem = (span > (uint8_t)(n * hz_w)) ? (uint8_t)(span - n * hz_w) : 0u;
		gap = (uint8_t)(rem / (n + 1u));
		extra = (uint8_t)(rem % (n + 1u));
		x = (uint8_t)(x0 + gap);
		for (i = 0; i < n; i++) {
			char dig[2];
			char hz[2];
			uint8_t dx;
			dig[0] = (char)('1' + i);
			dig[1] = '\0';
			hz[0] = cand->hanzi[i * 2u];
			hz[1] = cand->hanzi[i * 2u + 1u];
			msg_draw_cn16_char_at_y(hz, x, y);
			dx = (x > x0 + dig_w) ? (uint8_t)(x - dig_w) : x0;
			msg_draw_small_at_y(dig, dx, (uint8_t)(y + 4u), false);
			x = (uint8_t)(x + hz_w + gap + (i < extra ? 1u : 0u));
		}
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

static void print_home_item(const char *en_label, uint8_t y, bool sel)
{
	const char *t = msg_label(en_label);
	if (msg_lang_cn() && UI_StringHasGb2312(t))
		msg_draw_cn8_at_y(t, 1, y, sel);
	else
		print_line_y(t, y, sel);
}

static void print_wrapped_small_y(const char *s, uint8_t y, uint8_t max_lines)
{
	char linebuf[40];
	uint8_t line = 0;
	const size_t len = strlen(s);
	size_t i = 0;

	while (i < len && line < max_lines) {
		unsigned w = 0;
		size_t start = i;
		size_t n = 0;

		while (i < len) {
			const uint8_t c = (uint8_t)s[i];
			unsigned cw;
			size_t step = 1;

			if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len &&
			    (uint8_t)s[i + 1] >= 0xA1 && (uint8_t)s[i + 1] <= 0xFE) {
				cw = FONT8_SIZE;
				step = 2;
			} else {
				cw = 7u;
			}
			if (w + cw > 126u && n > 0)
				break;
			w += cw;
			i += step;
			n += step;
			if (n >= sizeof(linebuf) - 1u)
				break;
		}
		if (n == 0) {
			/* Single glyph wider than line — force one unit. */
			n = ((uint8_t)s[i] >= 0xA1 && (i + 1) < len) ? 2u : 1u;
			i += n;
		}
		if (n >= sizeof(linebuf))
			n = sizeof(linebuf) - 1u;
		memcpy(linebuf, s + start, n);
		linebuf[n] = 0;
		msg_draw_cn8_at_y(linebuf, 0, (uint8_t)(y + (line * 8U)), false);
		line++;
	}
}

/* Like print_wrapped_small_y, but shows the last max_lines (tail) so the
 * caret / newest text stays visible when the body area is shrunk for IME. */
static void print_wrapped_small_y_tail(const char *s, uint8_t y, uint8_t max_lines)
{
	size_t starts[12];
	uint8_t nlines = 0;
	const size_t len = strlen(s);
	size_t i = 0;
	uint8_t first;
	uint8_t row;
	char linebuf[40];

	if (max_lines == 0)
		return;
	if (len == 0)
		return;

	while (i < len && nlines < (uint8_t)ARRAY_SIZE(starts)) {
		unsigned w = 0;
		size_t n = 0;

		starts[nlines++] = i;
		while (i < len) {
			const uint8_t c = (uint8_t)s[i];
			unsigned cw;
			size_t step = 1;

			if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len &&
			    (uint8_t)s[i + 1] >= 0xA1 && (uint8_t)s[i + 1] <= 0xFE) {
				cw = FONT8_SIZE;
				step = 2;
			} else {
				cw = 7u;
			}
			if (w + cw > 126u && n > 0)
				break;
			w += cw;
			i += step;
			n += step;
			if (n >= sizeof(linebuf) - 1u)
				break;
		}
		if (n == 0) {
			n = ((uint8_t)s[i] >= 0xA1 && (i + 1) < len) ? 2u : 1u;
			i += n;
		}
	}

	first = (nlines > max_lines) ? (uint8_t)(nlines - max_lines) : 0u;
	for (row = 0; first + row < nlines && row < max_lines; row++) {
		size_t start = starts[first + row];
		size_t end = (first + row + 1u < nlines) ? starts[first + row + 1u] : len;
		size_t n = end - start;
		if (n >= sizeof(linebuf))
			n = sizeof(linebuf) - 1u;
		memcpy(linebuf, s + start, n);
		linebuf[n] = 0;
		msg_draw_cn8_at_y(linebuf, 0, (uint8_t)(y + (row * 8U)), false);
	}
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

static void draw_home_icon(uint8_t idx)
{
    /* Icons share one center point in the right side, midway between the
     * INBOX and DRAFTS rows.  They are not tied to the bottom separator.
     * CN: list up 3px vs prior CN layout → icon dy 6-3=3. */
    const uint8_t dy = msg_lang_cn() ? 1U : 0U;
    switch (idx) {
        case 0: draw_icon_envelope(88U, (uint8_t)(17U + dy)); break;  /* 30x18, center y~26 */
        case 1: draw_icon_pencil(89U, (uint8_t)(13U + dy)); break;    /* 30x28, center y~27 */
        case 2: draw_icon_up_arrow(88U, (uint8_t)(14U + dy)); break;  /* 31x27, center y~27 */
        default: draw_icon_floppy(86U, (uint8_t)(15U + dy)); break;   /* 34x24, visual center y~27 */
    }
}

static void draw_home(void)
{
    static const char *items[] = { "INBOX", "COMPOSE", "SENT", "DRAFTS" };
    char buf[24];
    /* CN: list/sep/SELECT block up 2px (row0 14→12, sep 49→47, sel 52→50). */
    const uint8_t row0 = msg_lang_cn() ? 12U : 10U;
    const uint8_t row_step = 9U;

    draw_title("MESSENGER");
    /* 0.3.0: HOME list shifted 1 px up so SELECT keeps 2 px bottom
     * clearance; right-side icon uses a fixed center shared by all states. */
    for (uint8_t i = 0; i < 4; i++) {
        print_home_item(items[i], (uint8_t)(row0 + (i * row_step)), gMsgHomeCursor == i);
    }
    draw_home_icon(gMsgHomeCursor);

    {
        const uint8_t sep_y = msg_lang_cn() ? 47U : 46U;
        const uint8_t sel_y = msg_lang_cn() ? 50U : 49U;
        draw_dotted_separator(sep_y);
        GUI_DisplaySmallest("SELECT", 0, sel_y, false, true);
    }

    if (gMessengerConfig.msg_debug) {
        /* RF22 ACK debug replaces old RF counter debug to save screen space.
         * P=pending MsgID, A=last ACK id heard, R=ACK rx count, M=match count. */
        snprintf(buf, sizeof(buf), "P%04X A%04X R%u M%u",
                 MSG_RF_GetAckDbgPendingId(), MSG_RF_GetAckDbgRxId(),
                 MSG_RF_GetAckDbgRxCount(), MSG_RF_GetAckDbgMatchCount());
        UI_PrintStringSmallNormal(buf, 0, 0, 6);
    }
}

static MSG_Message_t *current_list(uint8_t *count, const char **title)
{
    if (gMsgScreen == MSG_SCREEN_INBOX) { *count = MSG_STORE_CountInbox(); *title = "INBOX"; return gMessengerInbox; }
    if (gMsgScreen == MSG_SCREEN_OUTBOX) { *count = MSG_STORE_CountOutbox(); *title = "SENT"; return gMessengerOutbox; }
    *count = MSG_DRAFT_CAPACITY; *title = "DRAFTS"; return 0;
}

static void draw_list(void)
{
    uint8_t count; const char *title;
    MSG_Message_t *list = current_list(&count, &title);
    char buf[24];
    draw_title(title);
    snprintf(buf, sizeof(buf), "%u/%u", count ? (gMsgCursor + 1) : 0, count);
    print_right_small(buf, 0);
    if (!count) { UI_PrintStringSmallNormal("EMPTY", 0, 0, 3); return; }
    for (uint8_t row = 0; row < 6; row++) {
        uint8_t idx = gMsgScroll + row;
        if (idx >= count) break;
        if (gMsgScreen == MSG_SCREEN_DRAFTS) snprintf(buf, sizeof(buf), "%u %.18s", idx + 1, gMessengerConfig.drafts[idx]);
        else if (gMsgScreen == MSG_SCREEN_OUTBOX) {
            char st = '?';
            char age[5];
            format_age(list[idx].age_seconds, age, sizeof(age));
            if (list[idx].status == MSG_STATUS_ACKED) st = '+';
            else if (list[idx].status == MSG_STATUS_FAILED) st = 'x';
            snprintf(buf, sizeof(buf), "%c%-11.11s%4s", st, list[idx].text, age);
        } else {
            char age[5];
            format_age(list[idx].age_seconds, age, sizeof(age));
            snprintf(buf, sizeof(buf), "%c%-11.11s%4s", (list[idx].unread ? '*' : ' '), list[idx].text, age);
        }
        print_line(buf, row + 1, idx == gMsgCursor);
    }
}

static void draw_read(void)
{
    MSG_Message_t *m = (gMsgReadSource == MSG_SCREEN_OUTBOX) ? &gMessengerOutbox[gMsgReadIndex] : &gMessengerInbox[gMsgReadIndex];
    char buf[32];
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

        /* Metadata is pixel-positioned: one pixel lower than 0.2.4 so it
         * visually aligns with the large ACK marker and sits closer to the
         * upper separator. */
        GUI_DisplaySmallest(buf, 0, 9, false, true);
        char stbuf[2] = { st, 0 };
        UI_PrintStringSmallBold(stbuf, 120, 0, 1);
    } else {
        if (m->ttl_init == 0U) snprintf(buf, sizeof(buf), "FROM:%s HOP:OFF %s", m->from, age);
        else snprintf(buf, sizeof(buf), "FROM:%s HOP:%u/%u %s", m->from, used_hops, m->ttl_init, age);
        GUI_DisplaySmallest(buf, 0, 9, false, true);
    }

    /* 0.2.6: tighter message box.  Metadata moved down 1 px, message text
     * begins 4 px higher than before, and footer labels move up 1 px while
     * keeping the real-LCD safe area. */
    draw_dotted_separator(17);
    if (gMsgReadSource == MSG_SCREEN_OUTBOX && m->ack_count > 0u) {
        print_wrapped_small_y(m->text, 20, 2);
        char ackbuf[32];
        uint8_t pos = 0u;
        pos += (uint8_t)snprintf(ackbuf + pos, sizeof(ackbuf) - pos, "ACK:");
        for (uint8_t i = 0u; i < m->ack_count && i < MSG_ACK_SOURCE_MAX && pos < sizeof(ackbuf); i++) {
            pos += (uint8_t)snprintf(ackbuf + pos, sizeof(ackbuf) - pos, "%s%.*s", i ? " " : "", MSG_ACK_ID_LEN, m->ack_from[i]);
        }
        GUI_DisplaySmallest(ackbuf, 0, 40, false, true);
    } else {
        print_wrapped_small_y(m->text, 20, 3);
    }
    draw_dotted_separator(46);

    if (gMsgReadSource == MSG_SCREEN_OUTBOX) {
        GUI_DisplaySmallest("RESEND", 0, 49, false, true);
    } else {
        GUI_DisplaySmallest("RE:", 0, 49, false, true);
    }
    GUI_DisplaySmallest("F:DEL", 104, 49, false, true);
}

static void draw_compose(void)
{
    char buf[12];
    uint8_t body_lines = 3;
    PY_CandView_t cand;
    const bool py = (gMsgEditor.mode == 3U);

    draw_title("COMPOSE");

    GUI_DisplaySmallest("NEW MESSAGE", 0, 10, false, true);
    snprintf(buf, sizeof(buf), "%u/%u", (uint8_t)strlen(gMsgComposeBuf), (uint8_t)MSG_TEXT_LEN);
    {
        uint8_t w = (uint8_t)(strlen(buf) * 4U);
        uint8_t x = (w >= 128U) ? 0U : (uint8_t)(127U - w);
        GUI_DisplaySmallest(buf, x, 10, false, true);
    }

    draw_dotted_separator(17);

    memset(&cand, 0, sizeof(cand));
    if (py) {
        PY_GetCandView(&cand);
        if (cand.phase == 2u) body_lines = 1u;
        else if (cand.phase == 1u) body_lines = 2u;
    }

    print_wrapped_small_y_tail(gMsgComposeBuf, 20, body_lines);

    if (py && cand.phase >= 1u) {
        if (cand.phase == 2u) {
            /* Pinyin strip + 16×16 candidates above footer (fb y ends at 55). */
            msg_draw_py_sylls(0, 127, 27, &cand);
            msg_draw_py_cands16(0, 127, 34, &cand);
            GUI_DisplaySmallest("SEND", 0, 51, false, true);
            GUI_DisplaySmallest("pinyin", 92, 51, false, true);
            return;
        }
        {
            const uint8_t cy = (uint8_t)(20u + body_lines * 8u);
            msg_draw_py_sylls(0, 127, cy, &cand);
        }
    }

    draw_dotted_separator(46);
    GUI_DisplaySmallest("SEND", 0, 49, false, true);
    if (py)
        GUI_DisplaySmallest("pinyin", 92, 49, false, true);
    else
        GUI_DisplaySmallest((gMsgEditor.mode == 2U) ? "2" : (gMsgEditor.upper ? "B" : "b"), 120, 49, false, true);
}

static void draw_range(void)
{
    char buf[28];
    draw_title((gMsgRangeStatus == 1u || gMsgRangeStatus == 2u) ? "RANGE CHECK" : "HEARD");
    if (gMsgRangeStatus == 1u) GUI_DisplaySmallest("WAIT", 0, 1, false, true);
    else if (gMsgRangeStatus == 2u) GUI_DisplaySmallest("RESULT", 0, 1, false, true);

    const uint8_t top_sep = 9u;
    const uint8_t bottom_sep = 46u;
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
                msg_draw_small_at_y("WAITING", 40, 20, false);
                msg_draw_small_at_y("FOR PONG", 36, 29, false);
            } else {
                msg_draw_small_at_y("NOT FOUND", 33, 24, false);
            }
        } else {
            GUI_DisplaySmallest("FOUND:", 0, 12, false, true);
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
        GUI_DisplaySmallest("LAST HEARD:", 0, 12, false, true);
        uint8_t pages = 1u;
        if (gMsgRangeCount > 0u) pages = (uint8_t)((gMsgRangeCount + page_size - 1u) / page_size);
        if (gMsgRangeScroll >= pages) gMsgRangeScroll = (uint8_t)(pages - 1u);
        snprintf(buf, sizeof(buf), "%u/%u", (uint8_t)(gMsgRangeScroll + 1u), pages);
        uint8_t x = (uint8_t)(128u - (strlen(buf) * 4u));
        GUI_DisplaySmallest(buf, x, 12, false, true);

        if (gMsgRangeCount == 0u) {
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

    GUI_DisplaySmallest("PING", 0, 49, false, true);
    GUI_DisplaySmallest("EXIT", 112, 49, false, true);
}

static void draw_callsign(void)
{
    draw_title("MSG CSG");
    UI_PrintStringSmallNormal("CALLSIGN:", 0, 0, 1);
    UI_PrintStringSmallBold(gMsgCallsignBuf, 0, 0, 3);
    UI_PrintStringSmallNormal((gMsgCallsignEditor.mode == 2U) ? "2" : (gMsgCallsignEditor.upper ? "B" : "b"), 118, 0, 6);
}

static void draw_settings(void)
{
    const char *names[] = { "MSG RX", "MSG CSG", "CALLTX", "ACK", "HOP", "BEEP", "LED", "DEBUG", "TESTMSG", "BACK" };
    char buf[24];
    draw_title("MSG SET");
    uint8_t start = 0;
    if (gMsgSettingsCursor >= 5) start = (uint8_t)(gMsgSettingsCursor - 4U);
    for (uint8_t row = 0; row < 5; row++) {
        uint8_t idx = (uint8_t)(start + row);
        if (idx >= 10) break;
        switch (idx) {
            case 0: snprintf(buf, sizeof(buf), "%s:%s", names[idx], gMessengerConfig.msg_rx ? "ON" : "OFF"); break;
            case 1: snprintf(buf, sizeof(buf), "%s:%s", names[idx], gMessengerConfig.callsign); break;
            case 2: snprintf(buf, sizeof(buf), "%s:%s", names[idx], gMessengerConfig.callsign_tx ? "ON" : "OFF"); break;
            case 3: snprintf(buf, sizeof(buf), "%s:%s", names[idx], gMessengerConfig.msg_ack ? "ON" : "OFF"); break;
            case 4: snprintf(buf, sizeof(buf), "%s:%u", names[idx], gMessengerConfig.msg_hop); break;
            case 5: snprintf(buf, sizeof(buf), "%s:%s", names[idx], gMessengerConfig.msg_beep ? "ON" : "OFF"); break;
            case 6: snprintf(buf, sizeof(buf), "%s:%u", names[idx], gMessengerConfig.msg_led); break;
            case 7: snprintf(buf, sizeof(buf), "%s:%s", names[idx], gMessengerConfig.msg_debug ? "ON" : "OFF"); break;
            case 8: snprintf(buf, sizeof(buf), "%s", names[idx]); break;
            case 9: snprintf(buf, sizeof(buf), "%s", names[idx]); break;
            default: buf[0] = 0; break;
        }
        print_line(buf, row + 1, gMsgSettingsCursor == idx);
    }
    if (gMessengerConfig.msg_debug) {
        const uint8_t page = (uint8_t)((gFlashLightBlinkCounter / 64U) & 1U);
        if (page == 0) {
            snprintf(buf, sizeof(buf), "P%04X A%04X R%u M%u",
                     MSG_RF_GetAckDbgPendingId(), MSG_RF_GetAckDbgRxId(),
                     MSG_RF_GetAckDbgRxCount(), MSG_RF_GetAckDbgMatchCount());
        } else {
            snprintf(buf, sizeof(buf), "S%04X W%u T%u X%u",
                     MSG_RF_GetAckDbgSentId(), MSG_RF_GetAckDbgWaitActive(),
                     MSG_RF_GetAckDbgRetryCount(), MSG_RF_GetAckDbgMissCount());
        }
        UI_PrintStringSmallNormal(buf, 0, 0, 6);
    } else {
        snprintf(buf, sizeof(buf), "%u/10", (uint8_t)(gMsgSettingsCursor + 1));
        print_right_small(buf, 6);
    }
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
        case MSG_SCREEN_CALLSIGN: draw_callsign(); break;
        case MSG_SCREEN_SETTINGS: draw_settings(); break;
        case MSG_SCREEN_RANGE: draw_range(); break;
        default: draw_home(); break;
    }
    ST7565_BlitFullScreen();
    gUpdateStatus = true;
}
