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
/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <stdbool.h>
#include <string.h>
#include "app/scanner.h"
#include "dcs.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/scanner.h"

void UI_DisplayScanner(void)
{
    char String[28];
    char *pPrintStr = String;
    const bool cn = (gUiLanguage == UI_LANGUAGE_CN);

    UI_DisplayClear();

    /* Status line — 16×16 via UI_PrintString (GB2312 / gFontBig). */
    if (gScannerSaveState == SCAN_SAVE_CHANNEL) {
        pPrintStr = cn ? "\xB1\xA3\xB4\xE6\x3F" : "Save?"; /* 保存? */
    } else if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
        strcpy(String, cn ? "\xB1\xA3\xB4\xE6\x3A" : "Save:"); /* 保存: — both 5 bytes */
        UI_GenerateChannelStringEx(String + 5, gShowChPrefix, gScanChannel);
        pPrintStr = String;
    } else if ((gScanCssState < SCAN_CSS_STATE_FOUND) && ((gScanProgressIndicator & 1u) != 0)) {
        pPrintStr = "";
    } else if (gScanCssState == SCAN_CSS_STATE_OFF) {
        pPrintStr = cn ? "\xCB\xD1\xCB\xF7\xC6\xB5\xC2\xCA" : "Search Freq"; /* 搜索频率 */
    } else if (gScanCssState == SCAN_CSS_STATE_SCANNING) {
        pPrintStr = cn ? "\xCB\xD1\xCB\xF7\xD1\xC7\xD2\xF4" : "Search Tone"; /* 搜索亚音 */
    } else if (gScanCssState == SCAN_CSS_STATE_FOUND) {
        pPrintStr = cn ? "\xC9\xA8\xC3\xE8\xCD\xEA\xB3\xC9" : "Scan Complete"; /* 扫描完成 */
    } else {
        pPrintStr = cn ? "\xC9\xA8\xC3\xE8\xCA\xA7\xB0\xDC" : "Scan Failed"; /* 扫描失败 */
    }

    UI_PrintString(pPrintStr, 2, 0, 1, 8);

    if (gScanSingleFrequency || (gScanCssState != SCAN_CSS_STATE_OFF && gScanCssState != SCAN_CSS_STATE_FAILED)) {
        sprintf(String, cn ? "\xC6\xB5\xC2\xCA\x3A%u.%05u" : "Freq:%u.%05u",
                gScanFrequency / 100000, gScanFrequency % 100000); /* 频率: */
        pPrintStr = String;
    } else {
        pPrintStr = cn ? "\xC6\xB5\xC2\xCA\x3A---.-----" : "Freq:---.-----";
    }

    UI_PrintString(pPrintStr, 2, 0, 3, 8);

    if (gScanCssState < SCAN_CSS_STATE_FOUND) {
        pPrintStr = cn ? "\xD1\xC7\xD2\xF4\x3A---" : "Tone:---"; /* 亚音: */
    } else if (!gScanUseCssResult) {
        pPrintStr = cn ? "\xD1\xC7\xD2\xF4\x3A\xCE\xDE" : "Tone:None"; /* 亚音:无 */
    } else if (gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
        sprintf(String, "CTCSS:%u.%uHz", CTCSS_Options[gScanCssResultCode] / 10, CTCSS_Options[gScanCssResultCode] % 10);
        pPrintStr = String;
    } else {
        sprintf(String, "DCS:D%03oN", DCS_Options[gScanCssResultCode]);
        pPrintStr = String;
    }

    UI_PrintString(pPrintStr, 2, 0, 5, 8);

    ST7565_BlitFullScreen();
}
