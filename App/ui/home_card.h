/* Home card main screen (single / dual watch). */

#ifndef UI_HOME_CARD_H
#define UI_HOME_CARD_H

#include <stdint.h>
#include <stdbool.h>

void UI_DisplayHomeCard(void);
/* 500ms tick: refresh while RX / WFM, decay needle toward 0. true → redraw. */
bool UI_HomeCard_TimeSlice500ms(void);

#endif
