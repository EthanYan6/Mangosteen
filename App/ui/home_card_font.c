/* Home-card small font: same u8g2_font_5_tr as Dondji DualVfoU8g2_DrawSmallText. */

#include "ui/home_card_font.h"

#include <stddef.h>
#include <string.h>

#include "driver/st7565.h"
#include "ui/helper.h"
#include "u8g2.h"

#include "font_u8g2/font_5_tr.h"

static u8g2_t s_u8g2;
static uint8_t s_u8g2_ready;

static void prepare_small_font_text(const char *input_text, char *output_text, size_t output_capacity)
{
	if (output_text == NULL || output_capacity == 0u)
		return;

	output_text[0] = '\0';
	if (input_text == NULL)
		return;

	size_t read_index  = 0u;
	size_t write_index = 0u;

	while (input_text[read_index] != '\0' && write_index + 1u < output_capacity) {
		char mapped_char = input_text[read_index];

		if (mapped_char >= 'a' && mapped_char <= 'z')
			mapped_char = (char)(mapped_char - 'a' + 'A');

		if (mapped_char < 0x20 || mapped_char > 0x5f)
			mapped_char = ' ';

		output_text[write_index] = mapped_char;
		write_index++;
		read_index++;
	}

	output_text[write_index] = '\0';
}

static uint8_t u8x8_gpio_all_ok(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg,
                                U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
	return 1;
}

static void ensure_init(void)
{
	if (s_u8g2_ready)
		return;
	u8g2_Setup_st7565_64128n_f(&s_u8g2, U8G2_R0, u8x8_byte_empty, u8x8_gpio_all_ok);
	s_u8g2_ready = 1;
}

static void apply_text_rect(uint8_t (*buffer)[LCD_WIDTH], uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                            bool set_black)
{
	const uint8_t *buf = u8g2_GetBufferPtr(&s_u8g2);

	if (y0 > y1 || x0 > x1)
		return;

	for (uint8_t y = y0; y <= y1; y++) {
		const uint8_t row = (uint8_t)(y / 8u);
		const uint8_t bit = (uint8_t)(1u << (y % 8u));
		const uint16_t row_off = (uint16_t)row * 128u;
		for (uint8_t x = x0; x <= x1 && x < LCD_WIDTH; x++) {
			if (buf[row_off + x] & bit)
				UI_DrawPixelBuffer(buffer, x, y, set_black);
		}
	}
}

uint8_t HomeCardFont_GetSmallTextWidth(const char *text)
{
	if (text == NULL || text[0] == '\0')
		return 0u;

	char prepared_text[64];
	prepare_small_font_text(text, prepared_text, sizeof(prepared_text));
	if (prepared_text[0] == '\0')
		return 0u;

	ensure_init();
	u8g2_SetFont(&s_u8g2, u8g2_font_5_tr);
	const u8g2_uint_t width = u8g2_GetStrWidth(&s_u8g2, prepared_text);
	if (width > 255u)
		return 255u;

	return (uint8_t)width;
}

void HomeCardFont_DrawSmallText(const char *text, uint8_t x_left, uint8_t y_top, bool set_black)
{
	if (text == NULL || text[0] == '\0')
		return;

	char prepared_text[64];
	prepare_small_font_text(text, prepared_text, sizeof(prepared_text));
	if (prepared_text[0] == '\0')
		return;

	ensure_init();
	u8g2_ClearBuffer(&s_u8g2);
	u8g2_SetDrawColor(&s_u8g2, 1);
	u8g2_SetFont(&s_u8g2, u8g2_font_5_tr);

	{
		const uint8_t baseline_y = (uint8_t)(y_top + 5u);
		u8g2_DrawStr(&s_u8g2, x_left, baseline_y, prepared_text);
	}

	{
		const uint8_t width = HomeCardFont_GetSmallTextWidth(prepared_text);
		if (width == 0u)
			return;

		const uint8_t x_right = (uint8_t)(x_left + width - 1u);
		const uint8_t y_bottom = (uint8_t)(y_top + 5u);
		apply_text_rect(gFrameBuffer, x_left, y_top, x_right, y_bottom, set_black);
	}
}

void HomeCardFont_DrawSmallTextStatus(const char *text, uint8_t x_left, uint8_t y_top, bool set_black)
{
	if (text == NULL || text[0] == '\0')
		return;

	char prepared_text[64];
	prepare_small_font_text(text, prepared_text, sizeof(prepared_text));
	if (prepared_text[0] == '\0')
		return;

	ensure_init();
	u8g2_ClearBuffer(&s_u8g2);
	u8g2_SetDrawColor(&s_u8g2, 1);
	u8g2_SetFont(&s_u8g2, u8g2_font_5_tr);

	{
		const uint8_t baseline_y = (uint8_t)(y_top + 5u);
		u8g2_DrawStr(&s_u8g2, x_left, baseline_y, prepared_text);
	}

	{
		const uint8_t width = HomeCardFont_GetSmallTextWidth(prepared_text);
		if (width == 0u)
			return;

		const uint8_t x_right = (uint8_t)(x_left + width - 1u);
		const uint8_t y_bottom = (uint8_t)(y_top + 5u);
		apply_text_rect(&gStatusLine, x_left, y_top, x_right, y_bottom, set_black);
	}
}
