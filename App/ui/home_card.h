/* Home card main screen (single / dual watch). */

#ifndef UI_HOME_CARD_H
#define UI_HOME_CARD_H

#include <stdint.h>
#include <stdbool.h>

void UI_DisplayHomeCard(void);
/* 500ms tick: refresh RSSI target for needle (crawl/ghosts on 10ms). true → redraw. */
bool UI_HomeCard_TimeSlice500ms(void);

/* Manual A/B swap: front card slides up, back card moves to front (~450ms). */
void UI_HomeCard_StartVfoSwapAnim(uint8_t outgoing_vfo);
/* Ends anim immediately if active. true → was animating (caller should redraw). */
bool UI_HomeCard_CancelVfoSwapAnim(void);
bool UI_HomeCard_IsVfoSwapAnimActive(void);
/* 10ms tick: A/B swap anim, needle crawl, ghost decay. true → redraw. */
bool UI_HomeCard_TimeSlice10ms(void);

#endif
