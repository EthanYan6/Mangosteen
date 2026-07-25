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
#ifndef APP_MESSENGER_UI_H
#define APP_MESSENGER_UI_H

#include <stdint.h>

void UI_DisplayMessenger(void);

extern uint8_t gMsgComposeScroll;
#define MSG_UI_ComposeResetScroll() do { gMsgComposeScroll = 0u; } while (0)

/* 16×16 wrap line count (for read-view scroll clamps). */
uint8_t MSG_UI_TextLineCount16(const char *s);

#endif
