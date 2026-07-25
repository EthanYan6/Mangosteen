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
#ifndef BITMAP_MANGOSTEEN_H
#define BITMAP_MANGOSTEEN_H

#include <stdint.h>

#define BITMAP_MANGOSTEEN_WIDTH  48
#define BITMAP_MANGOSTEEN_HEIGHT 48
#define BITMAP_MANGOSTEEN_PAGES  (BITMAP_MANGOSTEEN_HEIGHT / 8)

extern const uint8_t BITMAP_Mangosteen[BITMAP_MANGOSTEEN_PAGES * BITMAP_MANGOSTEEN_WIDTH];

#endif
