#ifndef APP_PINYIN_IME_H
#define APP_PINYIN_IME_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/keyboard.h"

/* Compact T9 GB2312 pinyin — CN engine only. Hosts own ASCII modes. */

#define PY_PAGE_CHNAME   7u
#define PY_PAGE_COMPOSE  8u
#define PY_HANZI_MAX     8u

void PY_Start(char *buf, uint8_t max_len, uint8_t buf_cap,
              bool pad_spaces, bool cursor_nav, uint8_t page_size);
void PY_Stop(void);
bool PY_IsActive(void);
void PY_ClearComposing(void);

bool PY_ProcessKey(KEY_Code_t key, bool pressed, bool held);
void PY_Backspace(void);

/* MENU: composing → enter hanzi pick; picking → consumed (no save);
 * idle → false (host saves/sends). */
bool PY_HandleConfirm(void);
bool PY_IsComposing(void);

/*
 * phase: 0 idle / 1 choose syllable (↑↓) / 2 choose hanzi (1–N or ↑↓ page)
 * hanzi[]: GB2312 pairs for current page; count ≤ page_size
 */
typedef struct {
	char     pinyin[16];
	char     hanzi[PY_HANZI_MAX * 2u];
	uint8_t  count;
	uint8_t  phase;
	uint8_t  page_size;
} PY_CandView_t;

void PY_GetCandView(PY_CandView_t *out);
int  PY_GetCursor(void);

#endif
