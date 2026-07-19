/* Small text for home card: Dondji dual-VFO font (u8g2_font_5_tr). */
#ifndef UI_HOME_CARD_FONT_H
#define UI_HOME_CARD_FONT_H

#include <stdint.h>
#include <stdbool.h>

/** Draw small text into gFrameBuffer. y_top is frame-buffer Y (0..55). */
void HomeCardFont_DrawSmallText(const char *text, uint8_t x_left, uint8_t y_top, bool set_black);

/** Draw small text into gStatusLine. y_top is status Y (0..7). */
void HomeCardFont_DrawSmallTextStatus(const char *text, uint8_t x_left, uint8_t y_top, bool set_black);

uint8_t HomeCardFont_GetSmallTextWidth(const char *text);

#endif
