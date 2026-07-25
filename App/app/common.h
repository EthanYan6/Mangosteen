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

#ifndef APP_COMMON_H
#define APP_COMMON_H

#include "functions.h"
#include "settings.h"
#include "ui/ui.h"

void COMMON_KeypadLockToggle();
/* Manual A/B: dual/cross-band switches (+ home-card anim); MAIN ONLY beeps and refuses. */
void COMMON_SwitchVFOs();
void COMMON_SwitchVFOMode();

#endif