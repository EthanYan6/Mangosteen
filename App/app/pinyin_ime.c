#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "app/pinyin_ime.h"
#include "driver/py25q16.h"

#define FLASH_PINYIN_BASE 0x100000u
#define FLASH_HANZI       0x101F00u
#define PY_MAX_MATCH      6u

static const uint16_t s_off[6] = { 0x0000u, 0x0170u, 0x0780u, 0x14B0u, 0x1C90u, 0x1E20u };
static const uint8_t  s_cnt[6] = { 23u, 97u, 211u, 126u, 25u, 3u };
static const uint8_t  s_dlen[6] = { 1u, 1u, 2u, 2u, 3u, 3u };

static uint8_t  digitBuf[6];
static uint8_t  digitCnt;
static uint16_t matchOff[PY_MAX_MATCH];
static uint8_t  matchCnt;
static char     hanziBuf[PY_HANZI_MAX * 2u];
static uint8_t  f; /* 0 idle, 1 syllable, 2 hanzi */
static uint8_t  p;
static uint8_t  h;
static uint8_t  candTotal;
static uint8_t  g_page; /* 7 ChName / 8 compose */

static char    *g_buf;
static uint8_t  g_len;
static uint8_t  g_cur;
static uint8_t  g_max;
static uint8_t  g_flags; /* b0=active b1=pad b2=cursor_nav */
static uint8_t  sym_i;   /* KEY_1 punctuation multitap index; 0xFF = idle */

static const char s_syms[] = ".,?!";

static void clearSym(void)
{
	sym_i = 0xFFu;
}

static bool isHz(const char *s, uint8_t i, uint8_t len)
{
	uint8_t a = (uint8_t)s[i], b;
	if (i + 1u >= len || a < 0xA1u || a > 0xF7u)
		return false;
	b = (uint8_t)s[i + 1u];
	return b >= 0xA1u && b <= 0xFEu;
}

static void clearComp(void)
{
	digitCnt = matchCnt = candTotal = 0;
	f = p = h = 0;
	memset(digitBuf, 0, sizeof(digitBuf));
	memset(hanziBuf, 0, sizeof(hanziBuf));
	clearSym();
}

static void padOut(void)
{
	if ((g_flags & 2u) == 0u || !g_buf)
		return;
	if (g_len < g_max)
		memset(g_buf + g_len, ' ', (size_t)(g_max - g_len));
	g_buf[g_max] = '\0';
}

static uint32_t digitVal(void)
{
	uint32_t v = 0;
	uint8_t i;
	for (i = 0; i < digitCnt; i++)
		v = v * 10u + digitBuf[i];
	return v;
}

static void findMatches(void)
{
	uint8_t  di;
	uint32_t base;
	uint16_t n;
	uint8_t  dlen;
	uint16_t i;
	uint32_t want;
	uint8_t  fb[4];

	matchCnt = 0;
	if (digitCnt == 0u || digitCnt > 6u)
		return;

	di   = (uint8_t)(digitCnt - 1u);
	base = FLASH_PINYIN_BASE + s_off[di];
	n    = s_cnt[di];
	dlen = s_dlen[di];
	want = digitVal();

	for (i = 0; i < n && matchCnt < PY_MAX_MATCH; i++) {
		uint32_t addr = base + (uint32_t)i * 16u;
		PY25Q16_ReadBuffer(addr, fb, dlen);
		if (memcmp(&want, fb, dlen) == 0)
			matchOff[matchCnt++] = (uint16_t)(addr - FLASH_PINYIN_BASE);
	}
	if (p >= matchCnt)
		p = 0;
}

static void loadHanzi(void)
{
	uint8_t  info[3];
	uint16_t offset;
	uint8_t  count, max_page, read_cnt;
	uint32_t addr;

	candTotal = 0;
	memset(hanziBuf, 0, sizeof(hanziBuf));
	if (matchCnt == 0u || p >= matchCnt || g_page == 0u)
		return;

	PY25Q16_ReadBuffer(FLASH_PINYIN_BASE + matchOff[p] + 12u, info, 3);
	offset = (uint16_t)info[0] | ((uint16_t)info[1] << 8);
	count  = info[2];
	max_page = (uint8_t)((count + g_page - 1u) / g_page);
	if (max_page == 0u)
		max_page = 1u;
	if (h >= max_page)
		h = (uint8_t)(max_page - 1u);

	addr = FLASH_HANZI + offset + (uint32_t)h * (uint32_t)g_page * 2u;
	read_cnt = (uint8_t)(count - h * g_page);
	if (read_cnt > g_page)
		read_cnt = g_page;
	if (read_cnt > PY_HANZI_MAX)
		read_cnt = PY_HANZI_MAX;
	candTotal = read_cnt;
	if (read_cnt)
		PY25Q16_ReadBuffer(addr, (uint8_t *)hanziBuf, (uint32_t)read_cnt * 2u);
}

static bool insertAt(const char *src, uint8_t n)
{
	uint8_t i;

	if (!g_buf || n == 0u || (uint16_t)g_len + n > g_max)
		return false;
	if (g_cur > g_len)
		g_cur = g_len;
	for (i = g_len; i > g_cur; ) {
		i--;
		g_buf[i + n] = g_buf[i];
	}
	for (i = 0; i < n; i++)
		g_buf[g_cur + i] = src[i];
	g_len = (uint8_t)(g_len + n);
	g_cur = (uint8_t)(g_cur + n);
	g_buf[g_len] = '\0';
	padOut();
	return true;
}

static uint8_t prevStart(uint8_t cur)
{
	uint8_t i = 0, prev = 0;
	while (i < cur) {
		prev = i;
		i = (uint8_t)(i + (isHz(g_buf, i, g_len) ? 2u : 1u));
	}
	return prev;
}

static void moveCur(int dir)
{
	if ((g_flags & 4u) == 0u || !g_buf)
		return;
	if (dir < 0) {
		if (g_cur)
			g_cur = prevStart(g_cur);
	} else if (g_cur < g_len) {
		g_cur = (uint8_t)(g_cur + (isHz(g_buf, g_cur, g_len) ? 2u : 1u));
	}
}

static void delChar(void)
{
	uint8_t start, w, i;

	if (!g_buf || g_cur == 0u)
		return;
	start = prevStart(g_cur);
	w = (uint8_t)(g_cur - start);
	for (i = start; i + w < g_len; i++)
		g_buf[i] = g_buf[i + w];
	g_len = (uint8_t)(g_len - w);
	g_cur = start;
	g_buf[g_len] = '\0';
	padOut();
}

static void readPy(uint8_t idx, char *dst, uint8_t cap)
{
	uint8_t buf[8];
	uint8_t k, n = 0;

	if (!dst || cap == 0u || idx >= matchCnt)
		return;
	dst[0] = '\0';
	PY25Q16_ReadBuffer(FLASH_PINYIN_BASE + matchOff[idx] + 4u, buf, 8);
	for (k = 0; k < 8u && n + 1u < cap; k++) {
		if (buf[k] == 0u || buf[k] == 0x20u)
			break;
		dst[n++] = (char)buf[k];
	}
	dst[n] = '\0';
}

static void enterPick(void)
{
	f = 2u;
	h = 0;
	loadHanzi();
}

static void onDigit(KEY_Code_t key)
{
	clearSym();
	if (f == 2u) {
		uint8_t sel = (uint8_t)(key - KEY_0);
		if (key >= KEY_1 && sel >= 1u && sel <= candTotal && sel <= g_page) {
			char hz[2];
			uint8_t si = (uint8_t)((sel - 1u) * 2u);
			hz[0] = hanziBuf[si];
			hz[1] = hanziBuf[si + 1u];
			insertAt(hz, 2);
		}
		clearComp();
		return;
	}
	if (key < KEY_2 || key > KEY_9)
		return;
	if (f == 0u)
		f = 1u;
	if (digitCnt >= 6u)
		return;
	digitBuf[digitCnt++] = (uint8_t)(key - KEY_0);
	findMatches();
	if (matchCnt == 0u) {
		digitCnt--;
		if (digitCnt == 0u)
			clearComp();
		else
			findMatches();
	}
	p = h = 0;
}

void PY_Start(char *buf, uint8_t max_len, uint8_t buf_cap,
              bool pad_spaces, bool cursor_nav, uint8_t page_size)
{
	uint8_t n;

	if (!buf || buf_cap < 2u)
		return;
	if (max_len == 0u || max_len >= buf_cap)
		max_len = (uint8_t)(buf_cap - 1u);
	if (page_size == 0u || page_size > PY_HANZI_MAX)
		page_size = PY_PAGE_COMPOSE;

	g_buf  = buf;
	g_max  = max_len;
	g_page = page_size;
	g_flags = (uint8_t)(1u | (pad_spaces ? 2u : 0u) | (cursor_nav ? 4u : 0u));

	n = (uint8_t)strlen(buf);
	if (n > max_len)
		n = max_len;
	while (n > 0u && buf[n - 1u] == ' ')
		n--;
	g_len = n;
	g_cur = n;
	buf[n] = '\0';
	padOut();
	clearComp();
}

void PY_Stop(void)
{
	g_flags = 0;
	g_buf = 0;
	g_len = g_cur = g_max = 0;
	g_page = PY_PAGE_COMPOSE;
	clearComp();
}

bool PY_IsActive(void)
{
	return (g_flags & 1u) != 0u;
}

void PY_ClearComposing(void)
{
	clearComp();
}

int PY_GetCursor(void)
{
	return (int)g_cur;
}

void PY_GetCandView(PY_CandView_t *out)
{
	uint8_t i;

	if (!out)
		return;
	memset(out, 0, sizeof(*out));
	out->page_size = g_page;
	if ((g_flags & 1u) == 0u || f == 0u || matchCnt == 0u)
		return;

	out->phase = f;
	out->syll_sel = p;

	if (f == 2u) {
		readPy(p, out->syll[0], sizeof(out->syll[0]));
		out->syll_n = 1u;
		out->count = candTotal;
		if (candTotal)
			memcpy(out->hanzi, hanziBuf, (size_t)candTotal * 2u);
		return;
	}

	/* phase 1: all matched syllables; host draws with gaps */
	out->syll_n = matchCnt;
	if (out->syll_n > PY_SYLL_MAX)
		out->syll_n = PY_SYLL_MAX;
	for (i = 0; i < out->syll_n; i++)
		readPy(i, out->syll[i], sizeof(out->syll[i]));
}

bool PY_IsComposing(void)
{
	return (g_flags & 1u) != 0u && f != 0u;
}

bool PY_HandleConfirm(void)
{
	if ((g_flags & 1u) == 0u || f == 0u)
		return false;
	if (f == 1u) {
		if (matchCnt > 0u)
			enterPick();
		return true;
	}
	/* Already picking — swallow MENU so host does not save/send. */
	return true;
}

void PY_Backspace(void)
{
	if ((g_flags & 1u) == 0u)
		return;
	if (f == 2u) {
		f = 1u;
		h = 0;
		candTotal = 0;
		return;
	}
	if (digitCnt > 0u) {
		digitCnt--;
		if (digitCnt == 0u)
			clearComp();
		else {
			findMatches();
			p = h = 0;
		}
		return;
	}
	delChar();
	clearSym();
}

bool PY_ProcessKey(KEY_Code_t key, bool pressed, bool held)
{
	if ((g_flags & 1u) == 0u || !pressed || held)
		return false;

	switch (key) {
		case KEY_0: {
			char sp = ' ';
			clearSym();
			insertAt(&sp, 1);
			clearComp();
			return true;
		}
		case KEY_1:
			if (f == 1u) {
				clearSym();
				enterPick();
				return true;
			}
			if (f == 2u) {
				onDigit(key);
				return true;
			}
			/* Idle: multitap punctuation (same role as letter-mode KEY_1). */
			{
				const uint8_t n = (uint8_t)(sizeof(s_syms) - 1u);
				if (sym_i != 0xFFu && g_cur > 0u && g_buf &&
				    !isHz(g_buf, (uint8_t)(g_cur - 1u), g_len)) {
					const char last = g_buf[g_cur - 1u];
					uint8_t k;
					bool is_sym = false;
					for (k = 0; k < n; k++) {
						if (s_syms[k] == last) {
							is_sym = true;
							break;
						}
					}
					if (is_sym) {
						sym_i = (uint8_t)((sym_i + 1u) % n);
						g_buf[g_cur - 1u] = s_syms[sym_i];
						return true;
					}
				}
				{
					char c = s_syms[0];
					if (insertAt(&c, 1))
						sym_i = 0u;
				}
				return true;
			}
		case KEY_2: case KEY_3: case KEY_4: case KEY_5:
		case KEY_6: case KEY_7: case KEY_8: case KEY_9:
			onDigit(key);
			return true;
		case KEY_UP:
			if (f == 0u) {
				moveCur(-1);
				return (g_flags & 4u) != 0u;
			}
			if (f == 1u) {
				if (p > 0u)
					p--;
				return true;
			}
			if (f == 2u) {
				if (h > 0u)
					h--;
				loadHanzi();
				return true;
			}
			return false;
		case KEY_DOWN:
			if (f == 0u) {
				moveCur(1);
				return (g_flags & 4u) != 0u;
			}
			if (f == 1u) {
				if (p + 1u < matchCnt)
					p++;
				return true;
			}
			if (f == 2u) {
				h++;
				loadHanzi();
				return true;
			}
			return false;
		default:
			return false;
	}
}
