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

#include <string.h>
#include <stdlib.h>

#include "../app/dtmf.h"
#include "../app/menu.h"
#include "../bitmaps.h"
#include "../board.h"
#include "../dcs.h"
#include "../driver/backlight.h"
#include "../driver/bk4819.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "../font.h"
#include "../frequencies.h"
#include "../helper/battery.h"
#include "../misc.h"
#include "../settings.h"
#ifdef ENABLE_MESSENGER
    #include "app/messenger_store.h"
#endif
#include "app/pinyin_ime.h"

#ifdef ENABLE_FEAT_F4HWN
    #include "../version.h"
#endif

#include "../driver/py25q16.h"
#include "helper.h"
#include "inputbox.h"
#include "menu.h"
#include "menu_sub_values_cn.h"
#include "ui.h"
#include "welcome.h"

#define SUBV(en, cn) ((gUiLanguage == UI_LANGUAGE_CN) ? (const char *)(cn) : (const char *)(en))

#ifndef FLASH_FONT16_BASE
#define FLASH_FONT16_BASE  0x0A0000u
#define FLASH_FONT8_BASE   0x0E0000u
#define FONT16_SIZE        32u
#define FONT8_SIZE         8u
#endif

const t_menu_item MenuList[] =
{
//   text,          menu ID
    {"Lang",        MENU_LANGUAGE      },
    {"Step",        MENU_STEP          },
    {"Power",       MENU_TXP           }, // was "TXP"
    {"RxDCS",       MENU_R_DCS         }, // was "R_DCS"
    {"RxCTCS",      MENU_R_CTCS        }, // was "R_CTCS"
    {"TxDCS",       MENU_T_DCS         }, // was "T_DCS"
    {"TxCTCS",      MENU_T_CTCS        }, // was "T_CTCS"
    {"TxODir",      MENU_SFT_D         }, // was "SFT_D"
    {"TxOffs",      MENU_OFFSET        }, // was "OFFSET"
    {"W/N",         MENU_W_N           },
#ifndef ENABLE_FEAT_F4HWN
    {"Scramb",      MENU_SCR           }, // was "SCR"
#endif
    {"BusyCL",      MENU_BCL           }, // was "BCL"
    {"Compnd",      MENU_COMPAND       },
    {"Mode",        MENU_AM            }, // was "AM"
#ifdef ENABLE_FEAT_F4HWN
    {"TXLock",      MENU_TX_LOCK       }, 
#endif
    {"ChList",      MENU_LIST_CH       },
    {"ChSave",      MENU_MEM_CH        }, // was "MEM-CH"
    {"ChDele",      MENU_DEL_CH        }, // was "DEL-CH"
    {"ChName",      MENU_MEM_NAME      },

    {"ScList",       MENU_S_LIST       },
    {"ScPri",        MENU_S_PRI        },
    {"PriCh1",       MENU_S_PRI_CH_1   },
    {"PriCh2",       MENU_S_PRI_CH_2   },
    {"ScnRev",      MENU_SC_REV        },
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_NOAA
        {"NOAA-S",      MENU_NOAA_S    },
    #endif
#endif
    {"F1Shrt",      MENU_F1SHRT        },
    {"F1Long",      MENU_F1LONG        },
    {"F2Shrt",      MENU_F2SHRT        },
    {"F2Long",      MENU_F2LONG        },
    {"M Long",      MENU_MLONG         },

    {"KeyLck",      MENU_AUTOLK        }, // was "AUTOLk"
    {"TxTOut",      MENU_TOT           }, // was "TOT"
    {"BatSav",      MENU_SAVE          }, // was "SAVE"
    {"BatTxt",      MENU_BAT_TXT       },
    {"Mic",         MENU_MIC           },
    {"MicBar",      MENU_MIC_BAR       },
    // {"ChDisp",      MENU_MDF           }, // was "MDF"
    {"POnMsg",      MENU_PONMSG        },
    {"BLTime",      MENU_ABR           }, // was "ABR"
    {"BLMin",       MENU_ABR_MIN       },
    {"BLMax",       MENU_ABR_MAX       },
    {"BLTxRx",      MENU_ABR_ON_TX_RX  },
    {"Beep",        MENU_BEEP          },
#ifdef ENABLE_VOICE
    {"Voice",       MENU_VOICE         },
#endif
    {"Roger",       MENU_ROGER         },
    {YAN_ID_NAME,   MENU_YAN_ID        },
    {"STE",         MENU_STE           },
    {"RP STE",      MENU_RP_STE        },
    {"1 Call",      MENU_1_CALL        },
#ifdef ENABLE_ALARM
    {"AlarmT",      MENU_AL_MOD        },
#endif
#ifdef ENABLE_DTMF_CALLING
    {"ANI ID",      MENU_ANI_ID        },
#endif
    {"UPCode",      MENU_UPCODE        },
    {"DWCode",      MENU_DWCODE        },
    {"PTT ID",      MENU_PTT_ID        },
    {"D ST",        MENU_D_ST          },
#ifdef ENABLE_DTMF_CALLING
    {"D Resp",      MENU_D_RSP         },
    {"D Hold",      MENU_D_HOLD        },
#endif
    {"D Prel",      MENU_D_PRE         },
#ifdef ENABLE_DTMF_CALLING
    {"D Decd",      MENU_D_DCD         },
    {"D List",      MENU_D_LIST        },
#endif
    {"D Live",      MENU_D_LIVE_DEC    }, // live DTMF decoder
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_AM_FIX
        {"AM Fix",      MENU_AM_FIX        },
    #endif
#endif
    {"VOX",         MENU_VOX           },
#ifdef ENABLE_FEAT_F4HWN
    {"SysInf",      MENU_VOL           }, // was "VOL"
#else
    {"BatVol",      MENU_VOL           }, // was "VOL"
#endif
    {"RxMode",      MENU_TDR           },
    {"Sql",         MENU_SQL           },
#ifdef ENABLE_FEAT_F4HWN
    {"SetPwr",      MENU_SET_PWR       },
    {"SetPTT",      MENU_SET_PTT       },
    {"SetTOT",      MENU_SET_TOT       },
    {"SetEOT",      MENU_SET_EOT       },
    {"SetCtr",      MENU_SET_CTR       },
    {"SetInv",      MENU_SET_INV       },
    {"SetLck",      MENU_SET_LCK       },
    // {"SetMet",      MENU_SET_MET       },
    // {"SetGUI",      MENU_SET_GUI       },
#ifdef ENABLE_FEAT_F4HWN_AUDIO    
    {"SetRxA",      MENU_SET_AUD       },
#endif
    {"SetTmr",      MENU_SET_TMR       },
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    {"SetOff",       MENU_SET_OFF      },
#endif
#ifdef ENABLE_FEAT_F4HWN_NARROWER
    {"SetNFM",      MENU_SET_NFM       },
#endif
#ifdef ENABLE_FEAT_F4HWN_VOL
    {"SetVol",      MENU_SET_VOL       },
#endif
#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    {"SetKey",      MENU_SET_KEY       },
#endif
#ifdef ENABLE_NOAA
    {"SetNWR",      MENU_NOAA_S    },
#endif
#ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
    {"SetScn",      MENU_SET_SCN       },
#endif
#ifdef ENABLE_FEAT_F4HWN_LOGO_SAV
    {"SetSav",      MENU_SET_SAV       },
#endif
#endif
#ifdef ENABLE_MESSENGER
    {"MsgRx",       MENU_MSG_RX        },
    {"MsgCsg",      MENU_MSG_CSG       },
    {"MsgAck",      MENU_MSG_ACK       },
    {"MsgBep",      MENU_MSG_BEEP      },
    {"MsgLed",      MENU_MSG_LED       },
    {"RngRsp",      MENU_RNG_RSP       },
#endif
    // hidden menu items from here on
    // enabled if pressing both the PTT and upper side button at power-on
#ifdef ENABLE_MESSENGER
    {"MsgDbg",      MENU_MSG_DEBUG     },
    {"MsgHop",      MENU_MSG_HOP       },
#endif
    {"F Lock",      MENU_F_LOCK        },
#ifndef ENABLE_FEAT_F4HWN
    {"Tx 200",      MENU_200TX         }, // was "200TX"
    {"Tx 350",      MENU_350TX         }, // was "350TX"
    {"Tx 500",      MENU_500TX         }, // was "500TX"
#endif
    {"350 En",      MENU_350EN         }, // was "350EN"
#ifndef ENABLE_FEAT_F4HWN
    {"ScraEn",      MENU_SCREN         }, // was "SCREN"
#endif
#ifdef ENABLE_F_CAL_MENU
    {"FrCali",      MENU_F_CALI        }, // reference xtal calibration
#endif
    {"BatCal",      MENU_BATCAL        }, // battery voltage calibration
    {"BatTyp",      MENU_BATTYP        }, // battery type 1600/2200mAh
    {"SetNav",      MENU_SET_NAV       }, // set navigation (LEFT / RIGHT or UP / DOWN)
    {"Reset",       MENU_RESET         }, // might be better to move this to the hidden menu items ?

    {"",                              0xff               }  // end of list - DO NOT delete or move this this
};

#ifdef ENABLE_MESSENGER
const uint8_t FIRST_HIDDEN_MENU_ITEM = MENU_MSG_DEBUG;
#else
const uint8_t FIRST_HIDDEN_MENU_ITEM = MENU_F_LOCK;
#endif

const char* const gSubMenu_TXP[] =
{
    "USER",
    "LOW 1",
    "LOW 2",
    "LOW 3",
    "LOW 4",
    "LOW 5",
    "MID",
    "HIGH"
};

const char* const gSubMenu_SFT_D[] =
{
    "OFF",
    "+",
    "-"
};

const char* const gSubMenu_W_N[] =
{
    "WIDE",
    "NARROW"
};

const char* const gSubMenu_OFF_ON[] =
{
    "OFF",
    "ON"
};

const char* gSubMenu_NA = "N/A";

const char* const gSubMenu_RXMode[] =
{
    "MAIN\nONLY",       // TX and RX on main only
    "DUAL RX\nRESPOND", // Watch both and respond
    "CROSS\nBAND",      // TX on main, RX on secondary
    "MAIN TX\nDUAL RX"  // always TX on main, but RX on both
};

#ifdef ENABLE_VOICE
    const char* const gSubMenu_VOICE[] =
    {
        "OFF",
        "CHI",
        "ENG"
    };
#endif

const char* const gSubMenu_MDF[] =
{
    "FREQ",
    "CHANNEL\nNUMBER",
    "NAME",
    "NAME\n+\nFREQ"
};

#ifdef ENABLE_ALARM
    const char* const gSubMenu_AL_MOD[] =
    {
        "SITE",
        "TONE"
    };
#endif

#ifdef ENABLE_DTMF_CALLING
const char* const gSubMenu_D_RSP[] =
{
    "DO\nNOTHING",
    "RING",
    "REPLY",
    "BOTH"
};
#endif

const char* const gSubMenu_PTT_ID[] =
{
    "OFF",
    "UP CODE",
    "DOWN CODE",
    "UP+DOWN\nCODE",
    "APOLLO\nQUINDAR"
};

const char* const gSubMenu_PONMSG[] =
{
#ifdef ENABLE_FEAT_F4HWN
    "NONE",
    "DEFAULT",
#ifdef ENABLE_FEAT_F4HWN_LOGO
    "LOGO",
#endif
#else
    "FULL",
    "MESSAGE",
    "VOLTAGE",
    "NONE"
#endif
};

#if defined(ENABLE_FEAT_F4HWN) && defined(ENABLE_FEAT_F4HWN_LOGO_SAV)
const char* const gSubMenu_SET_SAV[] =
{
    "OFF",
    "LOGO",
    "LOGO+",
    "MATRIX"
};
#endif

const char* const gSubMenu_ROGER[] =
{
    "OFF",
    "ROGER",
    "MDC",
    YAN_ID_NAME
};

const char* const gSubMenu_RESET[] =
{
    "VFO",
    "ALL"
};

const char* const gSubMenu_F_LOCK[] =
{
    "DEFAULT+\n137-174\n400-470",
    "FCC HAM\n144-148\n420-450",
#ifdef ENABLE_FEAT_F4HWN_CA
    "CA HAM\n144-148\n430-450",
#endif
    "CE HAM\n144-146\n430-440",
    "GB HAM\n144-148\n430-440",
    "137-174\n400-430",
    "137-174\n400-438",
#ifdef ENABLE_FEAT_F4HWN_PMR
    "PMR 446",
#endif
#ifdef ENABLE_FEAT_F4HWN_GMRS_FRS_MURS
    "GMRS\nFRS\nMURS",
#endif
    "DISABLE\nALL",
    "UNLOCK\nALL",
};

const char* const gSubMenu_RX_TX[] =
{
    "OFF",
    "TX",
    "RX",
    "TX/RX"
};

const char* const gSubMenu_BAT_TXT[] =
{
    "NONE",
    "VOLTAGE",
    "PERCENT"
};

const char* const gSubMenu_BATTYP[] =
{
    "1600mAh K5",
    "2200mAh K5",
    "3500mAh K5",
    "1400mAh K1",
    "2500mAh K1"
};

const char* const gSubMenu_SET_NAV[] =
{
    "LEFT\nRIGHT\nUV-K1",
    "UP\nDOWN\nUV-K5(8)",
};

#ifndef ENABLE_FEAT_F4HWN
const char* const gSubMenu_SCRAMBLER[] =
{
    "OFF",
    "2600Hz",
    "2700Hz",
    "2800Hz",
    "2900Hz",
    "3000Hz",
    "3100Hz",
    "3200Hz",
    "3300Hz",
    "3400Hz",
    "3500Hz"
};
#endif

#ifdef ENABLE_FEAT_F4HWN
    const char* const gSubMenu_SET_PWR[] =
    {
        "< 20m",
        "125m",
        "250m",
        "500m",
        "1",
        "2",
        "5"
    };

    const char* const gSubMenu_SET_PTT[] =
    {
        "CLASSIC",
        "ONEPUSH"
    };

    const char* const gSubMenu_SET_TOT[] =  
    {
        "OFF",
        "SOUND",
        "VISUAL",
        "ALL"
    };

    const char* const gSubMenu_SET_LCK[] =
    {
        "KEYS",
        "KEYS\nACTIONS",
        "KEYS\nPTT",
        "KEYS\nACTIONS\nPTT"
    };

    const char* const gSubMenu_SET_MET[] =
    {
        "TINY",
        "CLASSIC"
    };

    #ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
        const char* const gSubMenu_SET_SCN[] =
        {
            "NORMAL",
            "FAST"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        const char* const gSubMenu_SET_AUD_FM[] =
        {
            "FLAT",
            "CLEAN",
            "MID",
            "BOOST",
            "MAX"
        };

        const char* const gSubMenu_SET_AUD_AM[] =
        {
            "SHARP",
            "STOCK",
            "OPEN"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_NARROWER
        const char* const gSubMenu_SET_NFM[] =
        {
            "NARROW",
            "NARROWER"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        const char* const gSubMenu_SET_KEY[] =
        {
            "KEY_MENU",
            "KEY_UP",
            "KEY_DOWN",
            "KEY_EXIT",
            "KEY_STAR"
        };
    #endif
#endif

const t_sidefunction gSubMenu_SIDEFUNCTIONS[] =
{
    {"NONE",            ACTION_OPT_NONE},
#ifdef ENABLE_FLASHLIGHT
    {"FLASH\nLIGHT",    ACTION_OPT_FLASHLIGHT},
#endif
    {"POWER",           ACTION_OPT_POWER},
    {"MONITOR",         ACTION_OPT_MONITOR},
    {"SCAN",            ACTION_OPT_SCAN},
#ifdef ENABLE_VOX
    {"VOX",             ACTION_OPT_VOX},
#endif
#ifdef ENABLE_ALARM
    {"ALARM",           ACTION_OPT_ALARM},
#endif
#ifdef ENABLE_FMRADIO
    {"FM RADIO",        ACTION_OPT_FM},
#endif
#ifdef ENABLE_TX1750
    {"1750Hz",          ACTION_OPT_1750},
#endif
    {"LOCK\nKEYPAD",    ACTION_OPT_KEYLOCK},
    {"VFO A\nVFO B",    ACTION_OPT_A_B},
    {"VFO\nMEM",        ACTION_OPT_VFO_MR},
    {"MODE",            ACTION_OPT_SWITCH_DEMODUL},
#ifdef ENABLE_BLMIN_TMP_OFF
    {"BLMIN\nTMP OFF",  ACTION_OPT_BLMIN_TMP_OFF},      //BackLight Minimum Temporary OFF
#endif
#ifdef ENABLE_FEAT_F4HWN
    {"RX MODE",         ACTION_OPT_RXMODE},
    {"MAIN ONLY",       ACTION_OPT_MAINONLY},
    {"PTT",             ACTION_OPT_PTT},
    {"WIDE\nNARROW",    ACTION_OPT_WN},
    {"MUTE",            ACTION_OPT_MUTE},
    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        {"RxA",            ACTION_OPT_RXA},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        {"POWER\nHIGH",    ACTION_OPT_POWER_HIGH},
        {"REMOVE\nOFFSET",  ACTION_OPT_REMOVE_OFFSET},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_BEAM
        {"BEAM",            ACTION_OPT_BEAM},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RXTX_LOG
        {"RF LOG",          ACTION_OPT_RXTX_LOG},
    #endif
#endif
#ifdef ENABLE_MESSENGER
    {"MESSENGER",       ACTION_OPT_MESSENGER},
    {"HEARD",           ACTION_OPT_HEARD},
#endif
};

const uint8_t gSubMenu_SIDEFUNCTIONS_size = ARRAY_SIZE(gSubMenu_SIDEFUNCTIONS);

bool    gIsInSubMenu;
uint8_t gMenuCursor;
int UI_MENU_GetCurrentMenuId() {
    if(gMenuCursor < ARRAY_SIZE(MenuList))
        return MenuList[gMenuCursor].menu_id;

    return MenuList[ARRAY_SIZE(MenuList)-1].menu_id;
}

uint8_t UI_MENU_GetMenuIdx(uint8_t id)
{
    for(uint8_t i = 0; i < ARRAY_SIZE(MenuList); i++)
        if(MenuList[i].menu_id == id)
            return i;
    return 0;
}

int32_t gSubMenuSelection;

// edit box
char    edit_original[17]; // a copy of the text before editing so that we can easily test for changes/difference
char    edit[17];
int     edit_index;
bool    edit_is_uppercase = false;
uint8_t edit_text_mode    = 0;

#ifndef ENABLE_CUSTOM_MENU_LAYOUT
static void UI_MENU_DrawTopRightRoundedBadge(const char *text, const uint8_t line, const bool center_in_area, const uint8_t area_x1, const uint8_t area_x2)
{
    const size_t length = strlen(text);
    const size_t char_pitch = ARRAY_SIZE(gFontSmall[0]) + 1u;
    const size_t text_width = length * char_pitch;
    const size_t capsule_span = text_width + 1u; // matches UI_PrintStringSmallNormalInverse x_end computation
    uint8_t text_x;

    if (length == 0 || line == 0 || line >= FRAME_LINES) {
        return;
    }

    if (center_in_area && area_x2 > area_x1 + 2u) {
        const uint8_t min_x = area_x1 + 1u;
        uint8_t max_x;
        const uint8_t area_width = area_x2 - area_x1 + 1u;

        if (capsule_span >= area_width) {
            text_x = min_x;
        } else {
            text_x = (uint8_t)(area_x1 + ((area_width - capsule_span) / 2u));
        }

        if (area_x2 > capsule_span) {
            max_x = (uint8_t)(area_x2 - capsule_span);
        } else {
            max_x = min_x;
        }

        if (max_x < min_x) {
            max_x = min_x;
        }
        if (text_x < min_x) {
            text_x = min_x;
        } else if (text_x > max_x) {
            text_x = max_x;
        }
    } else {
        if (capsule_span >= (LCD_WIDTH - 3u)) {
            text_x = 1u;
        } else {
            const uint8_t global_shift_right = 1u;
            const uint8_t base_text_x = (uint8_t)(LCD_WIDTH - capsule_span - 3u);
            const uint8_t max_text_x  = (uint8_t)(LCD_WIDTH - capsule_span - 1u);
            const uint16_t shifted_x = (uint16_t)base_text_x + global_shift_right;

            if (shifted_x > max_text_x) {
                text_x = max_text_x;
            } else {
                text_x = (uint8_t)shifted_x;
            }
        }
    }

    UI_PrintStringSmallNormalInverse(text, text_x, 0, line);
}
#endif /* !ENABLE_CUSTOM_MENU_LAYOUT */

#ifdef ENABLE_CUSTOM_MENU_LAYOUT
/* When set, UI_DisplayMenu only fills s_menu_fill_buf and returns (no blit). */
static bool s_menu_fill_only;
static char s_menu_fill_buf[64];

#define MENU_LIST_ROWS          5
#define MENU_LIST_ITEM_H        8
#define MENU_LIST_GAP           1   /* was 3; reduced by 2px */
#define MENU_LIST_PITCH         (MENU_LIST_ITEM_H + MENU_LIST_GAP)
#define MENU_LIST_FIRST_Y       9   /* 1px gap below separator at y=7 */
#define MENU_LIST_TEXT_H        7   /* gFontSmall uses bits 0..6 */
#define MENU_LIST_TEXT_H_BIG    16  /* selected CN row: 16x16 glyphs */
#define MENU_LIST_SEL_PAD       2   /* inverted pad above/below selected glyphs */
#define MENU_LIST_VALUE_MAX     10
#define MENU_SMALL_CHAR_PITCH   (ARRAY_SIZE(gFontSmall[0]) + 1u)
#define MENU_BIG_CHAR_PITCH     8u  /* matches UI_PrintString / PrintTitleBig */
#define MENU_FB_H               (FRAME_LINES * 8)

static void UI_MENU_CompactValue(const char *in, char *out, unsigned out_sz, unsigned prefer_line)
{
    unsigned j = 0;
    unsigned line = 0;
    unsigned i = 0;

    if (out_sz == 0)
        return;

    out[0] = '\0';
    if (in == NULL || in[0] == '\0')
        return;

    /* Skip to the preferred line (0 = first). */
    while (in[i] != '\0' && line < prefer_line) {
        while (in[i] >= 32)
            i++;
        while (in[i] != '\0' && in[i] < 32)
            i++;
        line++;
    }

    for (; in[i] != '\0' && in[i] != '\n' && j + 1u < out_sz; i++) {
        if (in[i] >= 32)
            out[j++] = in[i];
    }
    out[j] = '\0';

    /* Missing preferred line → fall back to the first printable line. */
    if (out[0] == '\0' && prefer_line != 0u) {
        UI_MENU_CompactValue(in, out, out_sz, 0u);
        return;
    }

    if (j > MENU_LIST_VALUE_MAX)
        out[MENU_LIST_VALUE_MAX] = '\0';
}

static unsigned UI_MENU_ListPreferLine(int menu_id)
{
    /* ChName value is "CH\nNAME\nFREQ" — list should show the name. */
    if (menu_id == MENU_MEM_NAME)
        return 1u;
    return 0u;
}

static unsigned UI_MENU_ValueLineCount(const char *in)
{
    unsigned lines = 0;
    unsigned i     = 0;

    if (in == NULL || in[0] == '\0')
        return 0;

    while (in[i] != '\0') {
        lines++;
        while (in[i] >= 32)
            i++;
        while (in[i] != '\0' && in[i] < 32)
            i++;
    }
    return lines;
}

static void UI_MENU_PrintTitleBig(const char *text, uint8_t start)
{
    const size_t length = strlen(text);
    uint8_t cur_x = start;

    for (size_t i = 0; i < length; i++) {
        const uint8_t c = (uint8_t)text[i];

        if (c >= 0xA1 && c <= 0xF7 && (i + 1) < length) {
            const uint8_t lo = (uint8_t)text[i + 1];
            if (lo >= 0xA1 && lo <= 0xFE && (unsigned)cur_x + 16u <= LCD_WIDTH) {
                const uint32_t idx  = (uint32_t)(c - 0xA1) * 94u + (lo - 0xA1);
                const uint32_t addr = FLASH_FONT16_BASE + idx * FONT16_SIZE;
                uint8_t cnDot[FONT16_SIZE];
                PY25Q16_ReadBuffer(addr, cnDot, FONT16_SIZE);
                memcpy(gStatusLine + cur_x, cnDot, 16);
                memcpy(gFrameBuffer[0] + cur_x, cnDot + 16, 16);
                cur_x = (uint8_t)(cur_x + 16u);
                i++;
                continue;
            }
        }

        if (c > ' ' && c < 127 && (unsigned)cur_x + 7u < LCD_WIDTH) {
            const unsigned int index = (unsigned int)(c - ' ' - 1);
            memcpy(gStatusLine + cur_x, &gFontBig[index][0], 7);
            memcpy(gFrameBuffer[0] + cur_x, &gFontBig[index][7], 7);
            cur_x = (uint8_t)(cur_x + 8u);
        } else if (c == ' ') {
            cur_x = (uint8_t)(cur_x + 8u);
        }
    }
}

static unsigned UI_MENU_TextPixelWidth(const char *text)
{
    const size_t len = strlen(text);
    const unsigned pitch = MENU_SMALL_CHAR_PITCH;
    unsigned text_w = 0;

    for (size_t k = 0; k < len; k++) {
        const uint8_t c = (uint8_t)text[k];
        if (c >= 0xA1 && c <= 0xF7 && (k + 1) < len) {
            const uint8_t lo = (uint8_t)text[k + 1];
            if (lo >= 0xA1 && lo <= 0xFE) {
                text_w += FONT8_SIZE;
                k++;
                continue;
            }
        }
        if (c > ' ' && c < 127)
            text_w += pitch;
    }
    return text_w;
}

static unsigned UI_MENU_TextPixelWidthBig(const char *text)
{
    const size_t len = strlen(text);
    unsigned text_w = 0;

    for (size_t k = 0; k < len; k++) {
        const uint8_t c = (uint8_t)text[k];
        if (c >= 0xA1 && c <= 0xF7 && (k + 1) < len) {
            const uint8_t lo = (uint8_t)text[k + 1];
            if (lo >= 0xA1 && lo <= 0xFE) {
                text_w += 16u;
                k++;
                continue;
            }
        }
        if (c > ' ' && c < 127)
            text_w += MENU_BIG_CHAR_PITCH;
        else if (c == ' ')
            text_w += MENU_BIG_CHAR_PITCH;
    }
    return text_w;
}

static void UI_MENU_PrintSmallAtY(const char *text, uint8_t x0, uint8_t x1, uint8_t y, uint8_t max_rows)
{
    const size_t   len = strlen(text);
    const unsigned pitch = MENU_SMALL_CHAR_PITCH;
    const unsigned text_w = UI_MENU_TextPixelWidth(text);
    uint8_t        x = x0;

    if (max_rows == 0 || max_rows > 8)
        max_rows = 8;

    if (x1 > x0 && text_w + 1u < (unsigned)(x1 - x0 + 1u))
        x = (uint8_t)(x0 + ((x1 - x0 + 1u) - text_w) / 2u);

    uint8_t cur_x = x;
    for (size_t i = 0; i < len; i++) {
        const uint8_t c = (uint8_t)text[i];

        if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len) {
            const uint8_t lo = (uint8_t)text[i + 1];
            if (lo >= 0xA1 && lo <= 0xFE) {
                const uint32_t idx  = (uint32_t)(c - 0xA1) * 94u + (lo - 0xA1);
                const uint32_t addr = FLASH_FONT8_BASE + idx * FONT8_SIZE;
                uint8_t cnDot[FONT8_SIZE];
                PY25Q16_ReadBuffer(addr, cnDot, FONT8_SIZE);

                for (uint8_t col = 0; col < FONT8_SIZE; col++) {
                    const uint8_t bits = cnDot[col];
                    const uint8_t px   = (uint8_t)(cur_x + col);
                    if (px >= LCD_WIDTH)
                        break;
                    for (uint8_t row = 0; row < max_rows; row++) {
                        if (bits & (1u << row)) {
                            const uint8_t py = (uint8_t)(y + row);
                            if (py < FRAME_LINES * 8)
                                UI_DrawPixelBuffer(gFrameBuffer, px, py, true);
                        }
                    }
                }
                cur_x = (uint8_t)(cur_x + FONT8_SIZE);
                i++;
                continue;
            }
        }

        if (c > ' ' && c < 127) {
            const unsigned index = (unsigned)(c - ' ' - 1);

            for (uint8_t col = 0; col < ARRAY_SIZE(gFontSmall[0]); col++) {
                const uint8_t bits = gFontSmall[index][col];
                const uint8_t px   = (uint8_t)(cur_x + col);

                if (px >= LCD_WIDTH)
                    break;

                for (uint8_t row = 0; row < max_rows; row++) {
                    if (bits & (1u << row)) {
                        const uint8_t py = (uint8_t)(y + row);
                        if (py < FRAME_LINES * 8)
                            UI_DrawPixelBuffer(gFrameBuffer, px, py, true);
                    }
                }
            }
            cur_x = (uint8_t)(cur_x + pitch);
        }
    }
}

static void UI_MENU_PrintSmallRightAtY(const char *text, uint8_t y, uint8_t max_rows)
{
    const unsigned width = UI_MENU_TextPixelWidth(text);
    uint8_t x = 2;

    if (width + 2u < LCD_WIDTH)
        x = (uint8_t)(LCD_WIDTH - 2u - width);

    UI_MENU_PrintSmallAtY(text, x, 0, y, max_rows);
}

static void UI_MENU_DrawGb2312_16(const char hz[2], uint8_t x, uint8_t y)
{
    const uint8_t c = (uint8_t)hz[0];
    const uint8_t lo = (uint8_t)hz[1];
    uint8_t cnDot[FONT16_SIZE];
    uint8_t col;

    if (c < 0xA1u || c > 0xF7u || lo < 0xA1u || lo > 0xFEu)
        return;
    PY25Q16_ReadBuffer(FLASH_FONT16_BASE + ((uint32_t)(c - 0xA1u) * 94u + (lo - 0xA1u)) * FONT16_SIZE,
                       cnDot, FONT16_SIZE);
    for (col = 0; col < 16u; col++) {
        const uint8_t bits_lo = cnDot[col];
        const uint8_t bits_hi = cnDot[col + 16u];
        const uint8_t px = (uint8_t)(x + col);
        uint8_t row;
        if (px >= LCD_WIDTH)
            break;
        for (row = 0; row < 16u; row++) {
            const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
            if (bits & (uint8_t)(1u << (row & 7u))) {
                const uint8_t py = (uint8_t)(y + row);
                if (py < FRAME_LINES * 8)
                    UI_DrawPixelBuffer(gFrameBuffer, px, py, true);
            }
        }
    }
}

static void UI_MENU_DrawPyCands16(uint8_t x0, uint8_t x1, uint8_t y, const PY_CandView_t *cand)
{
    const uint8_t dig_w = 6u;
    const uint8_t gap_dh = 2u;
    const uint8_t hz_w = 16u;
    const uint8_t unit = (uint8_t)(dig_w + gap_dh + hz_w);
    uint8_t n, span, i, gap, extra, x;

    if (!cand || cand->count == 0u)
        return;
    n = cand->count;
    if (n > PY_HANZI_MAX)
        n = PY_HANZI_MAX;
    span = (uint8_t)(x1 - x0 + 1u);

    if ((uint16_t)n * unit + (n + 1u) <= span) {
        uint8_t rem = (uint8_t)(span - n * unit);
        gap = (uint8_t)(rem / (n + 1u));
        extra = (uint8_t)(rem % (n + 1u));
        x = (uint8_t)(x0 + gap);
        for (i = 0; i < n; i++) {
            char dig[2];
            char hz[2];
            dig[0] = (char)('1' + i);
            dig[1] = '\0';
            hz[0] = cand->hanzi[i * 2u];
            hz[1] = cand->hanzi[i * 2u + 1u];
            UI_MENU_DrawGb2312_16(hz, (uint8_t)(x + dig_w + gap_dh), y);
            UI_MENU_PrintSmallAtY(dig, x, 0, (uint8_t)(y + 4u), 8);
            x = (uint8_t)(x + unit + gap + (i < extra ? 1u : 0u));
        }
        return;
    }

    {
        uint8_t rem = (span > (uint8_t)(n * hz_w)) ? (uint8_t)(span - n * hz_w) : 0u;
        gap = (uint8_t)(rem / (n + 1u));
        extra = (uint8_t)(rem % (n + 1u));
        x = (uint8_t)(x0 + gap);
        for (i = 0; i < n; i++) {
            char dig[2];
            char hz[2];
            uint8_t dx;
            dig[0] = (char)('1' + i);
            dig[1] = '\0';
            hz[0] = cand->hanzi[i * 2u];
            hz[1] = cand->hanzi[i * 2u + 1u];
            UI_MENU_DrawGb2312_16(hz, x, y);
            dx = (x > x0 + dig_w) ? (uint8_t)(x - dig_w) : x0;
            UI_MENU_PrintSmallAtY(dig, dx, 0, (uint8_t)(y + 4u), 8);
            x = (uint8_t)(x + hz_w + gap + (i < extra ? 1u : 0u));
        }
    }
}

static void UI_MENU_DrawPySylls(uint8_t x0, uint8_t x1, uint8_t y, const PY_CandView_t *cand)
{
    uint8_t n, i, x;

    if (!cand || cand->syll_n == 0u)
        return;
    n = cand->syll_n;
    if (n > PY_SYLL_MAX)
        n = PY_SYLL_MAX;
    x = x0;
    for (i = 0; i < n && x <= x1; i++) {
        char buf[10];
        uint8_t p = 0;
        uint8_t w;
        if (i == cand->syll_sel)
            buf[p++] = '>';
        strncpy(buf + p, cand->syll[i], sizeof(buf) - p - 1u);
        buf[sizeof(buf) - 1u] = '\0';
        UI_MENU_PrintSmallAtY(buf, x, 0, y, 8);
        w = (uint8_t)(strlen(buf) * MENU_SMALL_CHAR_PITCH);
        x = (uint8_t)(x + w + 1u); /* 1px gap between syllables */
    }
}

static void UI_MENU_PrintBigAtY(const char *text, uint8_t x0, uint8_t x1, uint8_t y, uint8_t max_rows)
{
    const size_t   len = strlen(text);
    const unsigned text_w = UI_MENU_TextPixelWidthBig(text);
    uint8_t        x = x0;
    uint8_t        ascii_dy = 0;

    if (max_rows == 0 || max_rows > 16)
        max_rows = 16;

    /* Mixed CN + ASCII: drop Latin/digits 1px to optically align with 16x16 楷体. */
    for (size_t k = 0; k < len; k++) {
        const uint8_t c = (uint8_t)text[k];
        if (c >= 0xA1 && c <= 0xF7 && (k + 1) < len) {
            const uint8_t lo = (uint8_t)text[k + 1];
            if (lo >= 0xA1 && lo <= 0xFE) {
                ascii_dy = 1u;
                break;
            }
        }
    }

    if (x1 > x0 && text_w + 1u < (unsigned)(x1 - x0 + 1u))
        x = (uint8_t)(x0 + ((x1 - x0 + 1u) - text_w) / 2u);

    uint8_t cur_x = x;
    for (size_t i = 0; i < len; i++) {
        const uint8_t c = (uint8_t)text[i];

        if (c >= 0xA1 && c <= 0xF7 && (i + 1) < len) {
            const uint8_t lo = (uint8_t)text[i + 1];
            if (lo >= 0xA1 && lo <= 0xFE) {
                const uint32_t idx  = (uint32_t)(c - 0xA1) * 94u + (lo - 0xA1);
                const uint32_t addr = FLASH_FONT16_BASE + idx * FONT16_SIZE;
                uint8_t cnDot[FONT16_SIZE];
                PY25Q16_ReadBuffer(addr, cnDot, FONT16_SIZE);

                for (uint8_t col = 0; col < 16u; col++) {
                    const uint8_t bits_lo = cnDot[col];
                    const uint8_t bits_hi = cnDot[col + 16u];
                    const uint8_t px      = (uint8_t)(cur_x + col);
                    if (px >= LCD_WIDTH)
                        break;
                    for (uint8_t row = 0; row < max_rows; row++) {
                        const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
                        const uint8_t bit  = (uint8_t)(row & 7u);
                        if (bits & (1u << bit)) {
                            const uint8_t py = (uint8_t)(y + row);
                            if (py < FRAME_LINES * 8)
                                UI_DrawPixelBuffer(gFrameBuffer, px, py, true);
                        }
                    }
                }
                cur_x = (uint8_t)(cur_x + 16u);
                i++;
                continue;
            }
        }

        if (c > ' ' && c < 127) {
            const unsigned index = (unsigned)(c - ' ' - 1);

            for (uint8_t col = 0; col < 7u; col++) {
                const uint8_t bits_lo = gFontBig[index][col];
                const uint8_t bits_hi = gFontBig[index][col + 7u];
                const uint8_t px      = (uint8_t)(cur_x + col);

                if (px >= LCD_WIDTH)
                    break;

                for (uint8_t row = 0; row < 16u; row++) {
                    const uint8_t out_row = (uint8_t)(row + ascii_dy);
                    if (out_row >= max_rows)
                        break;
                    const uint8_t bits = (row < 8u) ? bits_lo : bits_hi;
                    const uint8_t bit  = (uint8_t)(row & 7u);
                    if (bits & (1u << bit)) {
                        const uint8_t py = (uint8_t)(y + out_row);
                        if (py < FRAME_LINES * 8)
                            UI_DrawPixelBuffer(gFrameBuffer, px, py, true);
                    }
                }
            }
            cur_x = (uint8_t)(cur_x + MENU_BIG_CHAR_PITCH);
        } else if (c == ' ') {
            cur_x = (uint8_t)(cur_x + MENU_BIG_CHAR_PITCH);
        }
    }
}

static void UI_MENU_InvertPixelsY(uint8_t y0, uint8_t y1)
{
    for (uint8_t y = y0; y <= y1 && y < FRAME_LINES * 8; y++) {
        const uint8_t bit = (uint8_t)(1u << (y % 8));
        for (uint8_t x = 0; x < LCD_WIDTH; x++)
            gFrameBuffer[y / 8][x] ^= bit;
    }
}

static void UI_MENU_DrawEditCard(const char *title, const char *value, bool text_edit, uint8_t edit_len)
{
    char          buf[64];
    unsigned      vlines;
    unsigned      shown;
    unsigned      i;
    uint8_t       body_lines;
    uint8_t       content_h;
    uint8_t       title_y;
    uint8_t       val_y;
    int16_t       x1, y1, x2, y2;
    const bool    confirm = (gAskForConfirmation != 0);
    const uint8_t top_bar_h = 1; /* solid black row above title text */
    const uint8_t title_h   = 8;
    const uint8_t gap_h     = 3;

    if (text_edit && edit_len > 0) {
        /* Live edit buffer only — pad visually with spaces already in edit[]. */
        strncpy(buf, value != NULL ? value : "", sizeof(buf) - 1u);
        buf[sizeof(buf) - 1u] = '\0';
        if (buf[0] == '\0')
            strcpy(buf, "--");
        /* Name (8) + underline/caret pad (2) + case hint (8). */
        vlines = 1u;
        body_lines = 0; /* unused; height set via edit_body_h below */
    } else if (value == NULL || value[0] == '\0') {
        strcpy(buf, "--");
        vlines = 1;
        body_lines = (uint8_t)(vlines + (confirm ? 1u : 0u));
    } else {
        strncpy(buf, value, sizeof(buf) - 1u);
        buf[sizeof(buf) - 1u] = '\0';
        vlines = UI_MENU_ValueLineCount(buf);
        if (vlines == 0) {
            strcpy(buf, "--");
            vlines = 1;
        }
        if (vlines > 4u)
            vlines = 4u;
        body_lines = (uint8_t)(vlines + (confirm ? 1u : 0u));
    }

    if (text_edit && edit_len > 0) {
        PY_CandView_t cand_h;
        uint8_t       cand_extra = 0;
        uint8_t       edit_body_h;

        memset(&cand_h, 0, sizeof(cand_h));
        if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME && edit_text_mode == 3u && edit_index >= 0) {
            PY_GetCandView(&cand_h);
            if (cand_h.phase == 1u)
                cand_extra = 8u;
            else if (cand_h.phase == 2u)
                cand_extra = 24u; /* pinyin 8 + 16×16 cand row */
        }
        edit_body_h = (uint8_t)(18u + cand_extra); /* name + caret + mode + cands */
        content_h = (uint8_t)(top_bar_h + title_h + gap_h + edit_body_h + (confirm ? 8u : 0u));
    } else {
        content_h = (uint8_t)(top_bar_h + title_h + gap_h + body_lines * 8u);
    }

    x1 = 6;
    x2 = 121;

    y1 = 16; /* prefer starting near fb line 2 */
    if ((unsigned)y1 + content_h > (unsigned)(FRAME_LINES * 8u)) {
        if (content_h >= FRAME_LINES * 8u)
            y1 = 0;
        else
            y1 = (int16_t)(FRAME_LINES * 8u - content_h);
    }

    y2 = (int16_t)(y1 + content_h);
    if (y2 > (int16_t)(FRAME_LINES * 8 - 1))
        y2 = (int16_t)(FRAME_LINES * 8 - 1);

    for (int16_t y = y1; y <= y2; y++) {
        for (int16_t x = x1; x <= x2; x++)
            UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)x, (uint8_t)y, false);
    }

    /* Sides + bottom; top is a solid black bar. */
    UI_DrawLineBuffer(gFrameBuffer, x1, y1, x1, y2, true);
    UI_DrawLineBuffer(gFrameBuffer, x2, y1, x2, y2, true);
    UI_DrawLineBuffer(gFrameBuffer, x1, y2, x2, y2, true);
    UI_DrawLineBuffer(gFrameBuffer, x1, y1, x2, y1, true);
    UI_DrawLineBuffer(gFrameBuffer, x2 + 1, y1 + 1, x2 + 1, y2 + 1, true);
    UI_DrawLineBuffer(gFrameBuffer, x1 + 1, y2 + 1, x2 + 1, y2 + 1, true);

    title_y = (uint8_t)(y1 + top_bar_h);
    UI_MENU_PrintSmallAtY(title, (uint8_t)(x1 + 2), (uint8_t)(x2 - 1), title_y, 8);
    /* Invert title text band (below the top black bar). */
    for (uint8_t y = title_y; y < title_y + title_h; y++) {
        const uint8_t bit = (uint8_t)(1u << (y % 8));
        for (int16_t x = x1 + 1; x < x2; x++)
            gFrameBuffer[y / 8][x] ^= bit;
    }

    /* 3px gap under the title, then value lines. */
    val_y = (uint8_t)(y1 + top_bar_h + title_h + gap_h);

    if (text_edit && edit_len > 0 && edit_index >= 0) {
        const unsigned pitch  = MENU_SMALL_CHAR_PITCH;
        const unsigned text_w = (unsigned)edit_len * pitch;
        const uint8_t  area_x1 = (uint8_t)(x1 + 2);
        const uint8_t  area_x2 = (uint8_t)(x2 - 1);
        uint8_t        tx      = area_x1;
        char           name_line[17];
        unsigned       nlen;
        const uint8_t  caret_i = ((unsigned)edit_index < edit_len)
            ? (uint8_t)edit_index
            : (uint8_t)(edit_len > 0u ? edit_len - 1u : 0u);

        nlen = strlen(buf);
        if (nlen > edit_len)
            nlen = edit_len;
        if (nlen > sizeof(name_line) - 1u)
            nlen = sizeof(name_line) - 1u;
        memcpy(name_line, buf, nlen);
        name_line[nlen] = '\0';

        if (area_x2 > area_x1 && text_w + 1u < (unsigned)(area_x2 - area_x1 + 1u))
            tx = (uint8_t)(area_x1 + ((area_x2 - area_x1 + 1u) - text_w) / 2u);

        /* Left-align glyphs to the underline slots (not centered on trimmed text). */
        UI_MENU_PrintSmallAtY(name_line, tx, 0, val_y, 8);

        for (uint8_t ci = 0; ci < edit_len; ci++) {
            const uint8_t cx = (uint8_t)(tx + ci * pitch);
            const uint8_t glyph_w = (uint8_t)(ARRAY_SIZE(gFontSmall[0]));

            if (ci != caret_i) {
                if (edit[ci] != 'g' && edit[ci] != 'j')
                    UI_DrawLineBuffer(gFrameBuffer, cx, (int16_t)(val_y + 7),
                                      (int16_t)(cx + glyph_w - 1), (int16_t)(val_y + 7), true);
            } else {
                /* Caret under the active character. */
                UI_DrawLineBuffer(gFrameBuffer, (int16_t)(cx + 1), (int16_t)(val_y + 8),
                                  (int16_t)(cx + 3), (int16_t)(val_y + 8), true);
                UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 2), (uint8_t)(val_y + 7), true);
            }
        }

        {
            const int  mid       = UI_MENU_GetCurrentMenuId();
            const bool hide_case = (mid == MENU_UPCODE || mid == MENU_DWCODE);
            const bool chname    = (mid == MENU_MEM_NAME);
            PY_CandView_t cand;
            uint8_t cand_extra = 0;
            uint8_t mode_y = (uint8_t)(val_y + 10u);
            const char *mode_label;

            memset(&cand, 0, sizeof(cand));
            if (chname && edit_text_mode == 3u) {
                PY_GetCandView(&cand);
                if (cand.phase == 1u)
                    cand_extra = 8u;
                else if (cand.phase == 2u)
                    cand_extra = 24u;
            }

            if (!hide_case) {
                if (chname) {
                    switch (edit_text_mode) {
                        case 1:  mode_label = "ABC"; break;
                        case 2:  mode_label = "123"; break;
                        case 3:  mode_label = "pinyin"; break;
                        default: mode_label = "abc"; break;
                    }
                } else {
                    mode_label = edit_is_uppercase ? "ABC" : "abc";
                }
                UI_MENU_PrintSmallAtY(mode_label,
                                      (uint8_t)(x1 + 2), (uint8_t)(x2 - 1),
                                      mode_y, 8);
            }
            if (chname && cand.phase >= 1u) {
                uint8_t cy = (uint8_t)(mode_y + 8u);
                UI_MENU_DrawPySylls((uint8_t)(x1 + 2), (uint8_t)(x2 - 1), cy, &cand);
                if (cand.phase == 2u && cand.count > 0u) {
                    UI_MENU_DrawPyCands16((uint8_t)(x1 + 2), (uint8_t)(x2 - 1),
                                         (uint8_t)(cy + 8u), &cand);
                }
            }
            shown = 0; /* confirm uses absolute offset below */
            if (confirm) {
                const char *msg = (gAskForConfirmation == 1)
                    ? SUBV("SURE?", gSubMenu_SURE_CN)
                    : SUBV("WAIT!", gSubMenu_WAIT_CN);
                UI_MENU_PrintSmallAtY(msg, (uint8_t)(x1 + 2), (uint8_t)(x2 - 1),
                                      (uint8_t)(val_y + (hide_case ? 10u : (18u + cand_extra))), 8);
            }
        }
        return;
    } else {
        shown = 0;
        i     = 0;
        while (buf[i] != '\0' && shown < vlines) {
            const unsigned start = i;
            char           line[22];
            unsigned       len   = 0;

            while (buf[i] >= 32)
                i++;
            len = i - start;
            if (len >= sizeof(line))
                len = sizeof(line) - 1u;
            memcpy(line, buf + start, len);
            line[len] = '\0';

            while (buf[i] != '\0' && buf[i] < 32)
                i++;

            UI_MENU_PrintSmallAtY(line, (uint8_t)(x1 + 2), (uint8_t)(x2 - 1),
                                  (uint8_t)(val_y + shown * 8u), 8);
            shown++;
        }
    }

    if (confirm) {
        const char *msg = (gAskForConfirmation == 1)
            ? SUBV("SURE?", gSubMenu_SURE_CN)
            : SUBV("WAIT!", gSubMenu_WAIT_CN);
        UI_MENU_PrintSmallAtY(msg, (uint8_t)(x1 + 2), (uint8_t)(x2 - 1),
                              (uint8_t)(val_y + shown * 8u), 8);
    }
}

static void UI_MENU_DrawListStyle(const char *current_value)
{
    char          vals[MENU_LIST_ROWS][MENU_LIST_VALUE_MAX + 1];
    const int     count = (int)gMenuListCount;
    const int     sel   = (int)gMenuCursor;
    int           first;
    uint8_t       row;
    const uint8_t save_cur = gMenuCursor;
    const int32_t save_sel = gSubMenuSelection;
    const bool    save_sub = gIsInSubMenu;
    const char   *card_value = current_value;
    const int     cur_id     = MenuList[save_cur].menu_id;

    if (count <= 0)
        return;

    first = sel - (MENU_LIST_ROWS / 2);
    if (first < 0)
        first = 0;
    if (first + MENU_LIST_ROWS > count)
        first = count - MENU_LIST_ROWS;
    if (first < 0)
        first = 0;

    /*
     * CN selected row is taller (2+16+2 pad). On the last menu item it sits
     * in the bottom slot and would be clipped — pull first forward so fewer
     * rows sit above and the big selection fully fits.
     */
    if (gUiLanguage == UI_LANGUAGE_CN && sel == count - 1) {
        const uint8_t big_sel_h  = (uint8_t)(MENU_LIST_SEL_PAD + MENU_LIST_TEXT_H_BIG + MENU_LIST_SEL_PAD);
        const uint8_t avail_list = (uint8_t)(MENU_FB_H - MENU_LIST_FIRST_Y);
        uint8_t       max_vis    = 1u;

        if (avail_list > big_sel_h)
            max_vis = (uint8_t)(1u + (avail_list - big_sel_h) / MENU_LIST_PITCH);
        if (max_vis > MENU_LIST_ROWS)
            max_vis = MENU_LIST_ROWS;
        if (count > (int)max_vis)
            first = count - (int)max_vis;
        else
            first = 0;
    }

    memset(vals, 0, sizeof(vals));

    /* Fill-only nested calls may scribble the FB; we clear before painting. */
    for (row = 0; row < MENU_LIST_ROWS; row++) {
        const int idx = first + (int)row;

        if (idx >= count)
            break;

        if (idx == sel) {
            UI_MENU_CompactValue(current_value, vals[row], sizeof(vals[row]),
                                 UI_MENU_ListPreferLine(MenuList[idx].menu_id));
            continue;
        }

        gMenuCursor  = (uint8_t)idx;
        gIsInSubMenu = false;
        MENU_ShowCurrentSetting();
        s_menu_fill_only = true;
        UI_DisplayMenu();
        s_menu_fill_only = false;
        UI_MENU_CompactValue(s_menu_fill_buf, vals[row], sizeof(vals[row]),
                             UI_MENU_ListPreferLine(MenuList[idx].menu_id));
    }

    gMenuCursor       = save_cur;
    gSubMenuSelection = save_sel;
    gIsInSubMenu      = save_sub;

    UI_DisplayClear();
    UI_StatusClear();

    /* Title uses former status strip + fb line 0 (16px big font). */
    UI_MENU_PrintTitleBig(UI_MENU_GetHeaderTitle(), 2);
    {
        char     idx_str[12];
        unsigned len;
        unsigned width;
        uint8_t  x;

        sprintf(idx_str, "%u/%u", 1u + (unsigned)sel, (unsigned)count);
        len   = strlen(idx_str);
        width = len * 8u; /* same pitch as UI_MENU_PrintTitleBig */
        x     = 2;
        if (width + 2u < LCD_WIDTH)
            x = (uint8_t)(LCD_WIDTH - 2u - width);
        /* Same row / baseline as "menu", right-aligned. */
        UI_MENU_PrintTitleBig(idx_str, x);
    }

    /*
     * Title in status + fb line 0 (y0..7). Separator on y=7.
     * y=8 left blank; items from y=9: 8px row + 1px gap → five rows fit.
     * CN selected row grows to 16px glyphs (+ pads); later rows shift down.
     */
    UI_DrawLineBuffer(gFrameBuffer, 0, 7, LCD_WIDTH - 1, 7, true);

    {
        uint8_t y_cursor = MENU_LIST_FIRST_Y;

        for (row = 0; row < MENU_LIST_ROWS; row++) {
            const int     idx     = first + (int)row;
            const bool    is_sel  = (idx == sel);
            const bool    big_sel = is_sel && (gUiLanguage == UI_LANGUAGE_CN);
            const uint8_t y0      = y_cursor;
            uint8_t       avail;
            uint8_t       item_h;

            if (idx >= count || y0 >= MENU_FB_H)
                break;

            avail  = (uint8_t)(MENU_FB_H - y0);
            item_h = (avail < MENU_LIST_ITEM_H) ? avail : (uint8_t)MENU_LIST_ITEM_H;

            if (is_sel) {
                /*
                 * Selected: invert band with 2px pad above/below glyphs.
                 * Same MENU_LIST_GAP after the band as between normal rows.
                 */
                const uint8_t text_h   = big_sel ? (uint8_t)MENU_LIST_TEXT_H_BIG
                                                 : (uint8_t)MENU_LIST_TEXT_H;
                const uint8_t sel_need = (uint8_t)(MENU_LIST_SEL_PAD + text_h + MENU_LIST_SEL_PAD);
                const uint8_t sel_h    = (avail < sel_need) ? avail : sel_need;

                if (sel_h > (uint8_t)(MENU_LIST_SEL_PAD * 2u)) {
                    const uint8_t text_y    = (uint8_t)(y0 + MENU_LIST_SEL_PAD);
                    const uint8_t text_rows = (uint8_t)(sel_h - (MENU_LIST_SEL_PAD * 2u));

                    if (big_sel) {
                        /* Title only — hide value so long CN labels cannot collide. */
                        UI_MENU_PrintBigAtY(UI_MENU_GetMenuTitle(&MenuList[idx]), 2, 0, text_y, text_rows);
                    } else {
                        UI_MENU_PrintSmallAtY(UI_MENU_GetMenuTitle(&MenuList[idx]), 2, 0, text_y, text_rows);
                        if (vals[row][0] != '\0')
                            UI_MENU_PrintSmallRightAtY(vals[row], text_y, text_rows);
                    }
                    UI_MENU_InvertPixelsY(y0, (uint8_t)(y0 + sel_h - 1u));
                } else if (sel_h > 0u) {
                    /* Heavily clipped: draw whatever fits, still invert the band. */
                    if (big_sel) {
                        UI_MENU_PrintBigAtY(UI_MENU_GetMenuTitle(&MenuList[idx]), 2, 0, y0, sel_h);
                    } else {
                        UI_MENU_PrintSmallAtY(UI_MENU_GetMenuTitle(&MenuList[idx]), 2, 0, y0, sel_h);
                        if (vals[row][0] != '\0')
                            UI_MENU_PrintSmallRightAtY(vals[row], y0, sel_h);
                    }
                    UI_MENU_InvertPixelsY(y0, (uint8_t)(y0 + sel_h - 1u));
                }

                y_cursor = (uint8_t)(y0 + sel_h + MENU_LIST_GAP);
            } else {
                UI_MENU_PrintSmallAtY(UI_MENU_GetMenuTitle(&MenuList[idx]), 2, 0, y0, item_h);
                if (vals[row][0] != '\0')
                    UI_MENU_PrintSmallRightAtY(vals[row], y0, item_h);
                y_cursor = (uint8_t)(y0 + MENU_LIST_PITCH);
            }
        }
    }

    if (save_sub) {
        bool    text_edit = false;
        uint8_t edit_len  = 0;

        if (edit_index >= 0 && cur_id == MENU_MEM_NAME) {
            card_value = edit;
            text_edit  = true;
            edit_len   = 10;
        }
#ifdef ENABLE_MESSENGER
        else if (edit_index >= 0 && cur_id == MENU_MSG_CSG) {
            card_value = edit;
            text_edit  = true;
            edit_len   = MSG_CALLSIGN_EDIT_LEN;
        }
#endif
        else if (edit_index >= 0 && cur_id == MENU_YAN_ID) {
            card_value = edit;
            text_edit  = true;
            edit_len   = YAN_ID_LEN;
        }
        else if (edit_index >= 0 && (cur_id == MENU_UPCODE || cur_id == MENU_DWCODE)) {
            card_value = edit;
            text_edit  = true;
            edit_len   = 16;
        }
        UI_MENU_DrawEditCard(UI_MENU_GetMenuTitle(&MenuList[save_cur]), card_value, text_edit, edit_len);
    }

    ST7565_BlitStatusLine();
}
#endif

void UI_DisplayMenu(void)
{
    const unsigned int menu_list_width = 6; // max no. of characters on the menu list (left side)
    const unsigned int menu_item_x1    = (8 * menu_list_width) + 2;
    const unsigned int menu_item_x2    = LCD_WIDTH - 1;
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
    unsigned int       i;
#endif
    char               StringBuf[64];  // bigger cuz we can now do multi-line in one string (use '\n' char)
    char              *String = StringBuf;
    char               top_right_badge[16];

    const int m = UI_MENU_GetCurrentMenuId();

#ifdef ENABLE_DTMF_CALLING
    char               Contact[16];
#endif

#ifdef ENABLE_CUSTOM_MENU_LAYOUT
    if (s_menu_fill_only)
        String = s_menu_fill_buf;
    else
#endif
        UI_DisplayClear();

#ifndef ENABLE_CUSTOM_MENU_LAYOUT
#ifdef ENABLE_FEAT_F4HWN
    UI_DrawLineBuffer(gFrameBuffer, 48, 0, 48, 55, 1); // Be ware, status zone = 8 lines, the rest = 56 ->total 64

    for (uint8_t i = 0; i < 48; i += 2)
    {
        gFrameBuffer[5][i] = 0x40;
    }
#endif

        // original menu layout
    for (i = 0; i < 3; i++)
        if (gMenuCursor > 0 || i > 0)
            if ((gMenuListCount - 1) != gMenuCursor || i != 2)
                UI_PrintString(UI_MENU_GetMenuTitle(&MenuList[gMenuCursor + i - 1]), 0, 0, i * 2, 8);

    // invert the current menu list item pixels
    for (i = 0; i < (8 * menu_list_width); i++)
    {
        gFrameBuffer[2][i] ^= 0xFF;
        gFrameBuffer[3][i] ^= 0xFF;
    }

    // draw vertical separating dotted line
    for (i = 0; i < 7; i++)
        gFrameBuffer[i][(8 * menu_list_width) + 1] = 0xAA;

    // draw the little sub-menu triangle marker
    if (gIsInSubMenu)
        memcpy(gFrameBuffer[0] + (8 * menu_list_width) + 1, BITMAP_CurrentIndicator, sizeof(BITMAP_CurrentIndicator));

    // draw the menu index number/count
    sprintf(String, "%2u.%u", 1 + gMenuCursor, gMenuListCount);

    UI_PrintStringSmallNormal(String, 2, 0, 6);
#endif

    // **************

    String[0] = '\0';
    top_right_badge[0] = '\0';

    bool already_printed = false;

    /* Brightness is set to max in some entries of this menu. Return it to the configured brightness
       level the "next" time we enter here.I.e., when we move from one menu to another.
       It also has to be set back to max when pressing the Exit key. */

    BACKLIGHT_TurnOn();

    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
        uint8_t gaugeLine = 0;
        uint8_t gaugeMin = 0;
        uint8_t gaugeMax = 0;
    //#endif

    switch (m)
    {
        case MENU_LANGUAGE:
            strcpy(String, (gUiLanguage == UI_LANGUAGE_CN)
                ? gSubMenu_LANGUAGE_CN[gSubMenuSelection]
                : gSubMenu_LANGUAGE_EN[gSubMenuSelection]);
            break;

        case MENU_SQL:
            sprintf(String, "%d", gSubMenuSelection);
            break;

        case MENU_MIC:
            {   // display the mic gain in actual dB rather than just an index number
                const uint8_t mic = gMicGain_dB2[gSubMenuSelection];
                sprintf(String, "+%u.%udB", mic / 2, (mic % 2) * 5);

                gaugeLine = 4;
                gaugeMin = 0;
                gaugeMax = 8;
            }
            break;

        case MENU_MIC_BAR:
            #ifdef ENABLE_AUDIO_BAR
                strcpy(String, SUBV(gSubMenu_OFF_ON[gSubMenuSelection], gSubMenu_OFF_ON_CN[gSubMenuSelection]));
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_STEP: {
            uint16_t step = gStepFrequencyTable[FREQUENCY_GetStepIdxFromSortedIdx(gSubMenuSelection)];
            sprintf(String, "%d.%02ukHz", step / 100, step % 100);
            break;
        }

        case MENU_TXP:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, SUBV(gSubMenu_TXP[gSubMenuSelection], gSubMenu_TXP_CN[gSubMenuSelection]));
            }
            else
            {
                sprintf(String, "%s\n%sW", SUBV(gSubMenu_TXP[gSubMenuSelection], gSubMenu_TXP_CN[gSubMenuSelection]), gSubMenu_SET_PWR[gSubMenuSelection - 1]);
            }
            break;

        case MENU_R_DCS:
        case MENU_T_DCS:
            if (gSubMenuSelection == 0)
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            else if (gSubMenuSelection < 105)
                sprintf(String, "D%03oN", DCS_Options[gSubMenuSelection -   1]);
            else
                sprintf(String, "D%03oI", DCS_Options[gSubMenuSelection - 105]);
            break;

        case MENU_R_CTCS:
        case MENU_T_CTCS:
        {
            if (gSubMenuSelection == 0)
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            else
                sprintf(String, "%u.%uHz", CTCSS_Options[gSubMenuSelection - 1] / 10, CTCSS_Options[gSubMenuSelection - 1] % 10);
            break;
        }

        case MENU_SFT_D:
            strcpy(String, SUBV(gSubMenu_SFT_D[gSubMenuSelection], gSubMenu_SFT_D_CN[gSubMenuSelection]));
            break;

        case MENU_OFFSET:
            if (!gIsInSubMenu || gInputBoxIndex == 0)
            {
                sprintf(String, "%3d.%05u", gSubMenuSelection / 100000, abs(gSubMenuSelection) % 100000);
            }
            else
            {
                const char * ascii = INPUTBOX_GetAscii();
                sprintf(String, "%.3s.%.3s  ",ascii, ascii + 3);
            }

            UI_PrintString(String, menu_item_x1, menu_item_x2, 1, 8);
            UI_PrintString("MHz",  menu_item_x1, menu_item_x2, 3, 8);

            already_printed = true;
            break;

        case MENU_W_N:
            strcpy(String, SUBV(gSubMenu_W_N[gSubMenuSelection], gSubMenu_W_N_CN[gSubMenuSelection]));
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCR:
            strcpy(String, gSubMenu_SCRAMBLER[gSubMenuSelection]);
            #if 1
                if (gSubMenuSelection > 0 && gSetting_ScrambleEnable)
                    BK4819_EnableScramble(gSubMenuSelection - 1);
                else
                    BK4819_DisableScramble();
            #endif
            break;
#endif

        case MENU_VOX:
            #ifdef ENABLE_VOX
                sprintf(String, gSubMenuSelection == 0 ? SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]) : "%u", gSubMenuSelection);
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_ABR:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            }
            else if(gSubMenuSelection < 61)
            {
                sprintf(String, "%02dm:%02ds", (((gSubMenuSelection) * 5) / 60), (((gSubMenuSelection) * 5) % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(4, 1, 60, gSubMenuSelection);
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 60;
                //#endif
            }
            else
            {
                strcpy(String, "ON");
            }

            // Obsolete ???
            //if(BACKLIGHT_GetBrightness() < 4)
            //    BACKLIGHT_SetBrightness(4);
            break;

        case MENU_ABR_MIN:
        case MENU_ABR_MAX:
            sprintf(String, "%d", gSubMenuSelection);
            if(gIsInSubMenu)
                BACKLIGHT_SetBrightness(gSubMenuSelection);
            // Obsolete ???
            //else if(BACKLIGHT_GetBrightness() < 4)
            //    BACKLIGHT_SetBrightness(4);
            break;

        case MENU_AM:
            strcpy(String, gModulationStr[gSubMenuSelection]);
            break;

        case MENU_AUTOLK:
            if (gSubMenuSelection == 0)
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            else
            {
                sprintf(String, "%02dm:%02ds", ((gSubMenuSelection * 15) / 60), ((gSubMenuSelection * 15) % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(4, 1, 40, gSubMenuSelection);
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 40;
                //#endif
            }
            break;

        case MENU_COMPAND:
        case MENU_ABR_ON_TX_RX:
            strcpy(String, SUBV(gSubMenu_RX_TX[gSubMenuSelection], gSubMenu_RX_TX_CN[gSubMenuSelection]));
            break;

        #ifndef ENABLE_FEAT_F4HWN
            #ifdef ENABLE_AM_FIX
                case MENU_AM_FIX:
            #endif
        #endif
        case MENU_BCL:
        case MENU_BEEP:
        case MENU_STE:
        case MENU_D_ST:
#ifdef ENABLE_DTMF_CALLING
        case MENU_D_DCD:
#endif
        case MENU_D_LIVE_DEC:
        #ifdef ENABLE_NOAA
            case MENU_NOAA_S:
        #endif
#ifndef ENABLE_FEAT_F4HWN
        case MENU_350TX:
        case MENU_200TX:
        case MENU_500TX:
#endif
        case MENU_350EN:
#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCREN:
#endif
#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_TMR:
        case MENU_S_PRI:
#endif
#ifdef ENABLE_MESSENGER
        case MENU_MSG_RX:
        case MENU_MSG_CALLTX:
        case MENU_MSG_ACK:
        case MENU_MSG_BEEP:
        case MENU_RNG_RSP:
        case MENU_MSG_DEBUG:
#endif
        {
            uint8_t value = (uint8_t)gSubMenuSelection;
#ifdef ENABLE_MESSENGER
            /* Avoid first-render stale value when entering the
             * hidden Messenger menu before MENU_ShowCurrentSetting() refreshes. */
            if (!gIsInSubMenu) {
                MSG_STORE_Init();
                if (m == MENU_MSG_RX) value = gMessengerConfig.msg_rx;
                else if (m == MENU_MSG_CALLTX) value = gMessengerConfig.callsign_tx;
                else if (m == MENU_MSG_ACK) value = gMessengerConfig.msg_ack;
                else if (m == MENU_MSG_BEEP) value = gMessengerConfig.msg_beep;
                else if (m == MENU_RNG_RSP) value = gMessengerConfig.rng_rsp;
                else if (m == MENU_MSG_DEBUG) value = gMessengerConfig.msg_debug;
            }
#endif
            if (value > 1u) value = 1u;
            strcpy(String, SUBV(gSubMenu_OFF_ON[value], gSubMenu_OFF_ON_CN[value]));
            break;
        }

#if defined(ENABLE_FEAT_F4HWN) && defined(ENABLE_FEAT_F4HWN_LOGO_SAV)
        case MENU_SET_SAV:
            strcpy(String, gSubMenu_SET_SAV[gSubMenuSelection]);
            break;
#endif

#ifdef ENABLE_MESSENGER
        case MENU_MSG_CSG:
            MSG_STORE_Init();
            if (!gIsInSubMenu)
                edit_index = -1;
            if (edit_index < 0)
            {
                strncpy(String, gMessengerConfig.callsign, MSG_CALLSIGN_EDIT_LEN + 1);
                String[MSG_CALLSIGN_EDIT_LEN] = 0;
                UI_PrintString(String[0] ? String : "--", menu_item_x1, menu_item_x2, 2, 8);
            }
            else
            {
                UI_PrintString(edit, menu_item_x1, menu_item_x2, 2, 8);
                if (edit_index < MSG_CALLSIGN_EDIT_LEN)
                {
                    const uint8_t edit_len = MSG_CALLSIGN_EDIT_LEN;
                    uint8_t x = menu_item_x1;
                    if (menu_item_x2 > menu_item_x1)
                    {
                        x = menu_item_x1 + (uint8_t)((((menu_item_x2 - menu_item_x1) - (edit_len * 8)) + 1) / 2);
                    }
                    if (x > 0) x--;

                    for (uint8_t i = 0; i < edit_len; i++)
                    {
                        if (i != edit_index)
                        {
                            if (edit[i] != 'g' && edit[i] != 'j')
                                UI_DrawLineBuffer(gFrameBuffer, x, 29, x + 6, 29, 1);
                        }
                        else
                        {
                            UI_DrawLineBuffer(gFrameBuffer, x + 2, 30, x + 4, 30, 1);
                            UI_DrawPixelBuffer(gFrameBuffer, x + 3, 29, 1);
                        }
                        x += 8;
                    }
                    UI_PrintStringSmallNormal(edit_is_uppercase ? "ABC" : "abc", 77, 0, 4);
                }
            }
            already_printed = true;
            break;

        case MENU_MSG_HOP:
        {
            uint8_t value = (uint8_t)gSubMenuSelection;
            if (!gIsInSubMenu) { MSG_STORE_Init(); value = gMessengerConfig.msg_hop; }
            if (value == 0) strcpy(String, "OFF");
            else sprintf(String, "%u", value);
            break;
        }
        case MENU_MSG_LED:
            if (gSubMenuSelection == 0) strcpy(String, "OFF");
            else if (gSubMenuSelection == 1) strcpy(String, "GREEN");
            else strcpy(String, "YELLOW");
            break;
#endif

        case MENU_YAN_ID:
            if (!gIsInSubMenu)
                edit_index = -1;
            if (edit_index < 0)
            {
                strncpy(String, gEeprom.yan_id, YAN_ID_LEN + 1);
                String[YAN_ID_LEN] = 0;
                UI_PrintString(String[0] ? String : "--", menu_item_x1, menu_item_x2, 2, 8);
            }
            else
            {
                UI_PrintString(edit, menu_item_x1, menu_item_x2, 2, 8);
                if (edit_index < YAN_ID_LEN)
                {
                    const uint8_t edit_len = YAN_ID_LEN;
                    uint8_t x = menu_item_x1;
                    if (menu_item_x2 > menu_item_x1)
                    {
                        x = menu_item_x1 + (uint8_t)((((menu_item_x2 - menu_item_x1) - (edit_len * 8)) + 1) / 2);
                    }
                    if (x > 0) x--;

                    for (uint8_t i = 0; i < edit_len; i++)
                    {
                        if (i != edit_index)
                        {
                            if (edit[i] != 'g' && edit[i] != 'j')
                                UI_DrawLineBuffer(gFrameBuffer, x, 29, x + 6, 29, 1);
                        }
                        else
                        {
                            UI_DrawLineBuffer(gFrameBuffer, x + 2, 30, x + 4, 30, 1);
                            UI_DrawPixelBuffer(gFrameBuffer, x + 3, 29, 1);
                        }
                        x += 8;
                    }
                    UI_PrintStringSmallNormal(edit_is_uppercase ? "ABC" : "abc", 77, 0, 4);
                }
            }
            already_printed = true;
            break;

        case MENU_MEM_CH:
        case MENU_1_CALL:
        case MENU_DEL_CH:
        case MENU_S_PRI_CH_1:
        case MENU_S_PRI_CH_2:
        {
            if (gSubMenuSelection == MR_CHANNELS_MAX)
            {
                strcpy(String, "None");
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
                UI_PrintString("None", menu_item_x1, menu_item_x2, 2, 8);
#endif
                already_printed = true;
                break;
            }
            else
            {
                const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);
                char       ch_str[16];
                char       name[16];
                char       freq[16];

                UI_GenerateChannelStringEx(ch_str, valid, gSubMenuSelection);
                name[0] = '\0';
                freq[0] = '\0';

                if (valid) {
                    SETTINGS_FetchChannelName(name, gSubMenuSelection);
                    if (!gAskForConfirmation) {
                        const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);
                        sprintf(freq, "%u.%05u", frequency / 100000, frequency % 100000);
                    }
                }
                if (name[0] == '\0')
                    strcpy(name, "--");

#ifdef ENABLE_CUSTOM_MENU_LAYOUT
                if (freq[0] != '\0')
                    sprintf(String, "%s\n%s\n%s", ch_str, name, freq);
                else
                    sprintf(String, "%s\n%s", ch_str, name);
#else
                UI_PrintString(ch_str, menu_item_x1, menu_item_x2, 0, 8);
                UI_PrintString(name, menu_item_x1, menu_item_x2, 2, 8);
                if (freq[0] != '\0')
                    UI_PrintString(freq, menu_item_x1, menu_item_x2, 5, 8);
#endif
                already_printed = true;
                break;
            }
        }

        case MENU_MEM_NAME:
        {
            const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);
            char       ch_str[16];
            char       name[16];
            char       freq[16];

            UI_GenerateChannelStringEx(ch_str, valid, gSubMenuSelection);
            name[0] = '\0';
            freq[0] = '\0';

            if (!gIsInSubMenu)
                edit_index = -1;

            if (valid)
            {
                const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);

                if (edit_index < 0) {
                    SETTINGS_FetchChannelName(name, gSubMenuSelection);
                } else {
                    strncpy(name, edit, sizeof(name) - 1u);
                    name[sizeof(name) - 1u] = '\0';
                }
                if (!gAskForConfirmation)
                    sprintf(freq, "%u.%05u", frequency / 100000, frequency % 100000);
            }
            if (name[0] == '\0')
                strcpy(name, "--");

#ifdef ENABLE_CUSTOM_MENU_LAYOUT
            if (freq[0] != '\0')
                sprintf(String, "%s\n%s\n%s", ch_str, name, freq);
            else
                sprintf(String, "%s\n%s", ch_str, name);
#else
            UI_PrintString(ch_str, menu_item_x1, menu_item_x2, 0, 8);

            if (valid)
            {
                if (edit_index < 0) {
                    UI_PrintString(name, menu_item_x1, menu_item_x2, 2, 8);
                } else {
                    UI_PrintString(edit, menu_item_x1, menu_item_x2, 2, 8);
                    if (edit_index < 10) {
                        uint8_t x = menu_item_x1 - 1;
                        for (uint8_t i = 0; i < 10; i++)
                        {
                            if (i != edit_index)
                            {
                                if (edit[i] != 'g' && edit[i] != 'j')
                                    UI_DrawLineBuffer(gFrameBuffer, x, 29, x + 6, 29, 1);
                            }
                            else
                            {
                                UI_DrawLineBuffer(gFrameBuffer, x + 2, 30, x + 4, 30, 1);
                                UI_DrawPixelBuffer(gFrameBuffer, x + 3, 29, 1);
                            }
                            x += 8;
                        }
                        UI_PrintStringSmallNormal(edit_is_uppercase ? "ABC" : "abc", 77, 0, 4);
                    }
                }

                if (freq[0] != '\0')
                    UI_PrintString(freq, menu_item_x1, menu_item_x2, 5, 8);
            }
#endif
            already_printed = true;
            break;
        }

        case MENU_SAVE:
            sprintf(String, gSubMenuSelection == 0 ? SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]) : "1:%u", gSubMenuSelection);
            break;

        case MENU_TDR:
            strcpy(String, SUBV(gSubMenu_RXMode[gSubMenuSelection], gSubMenu_RXMode_CN[gSubMenuSelection]));
            break;

        case MENU_TOT:
            sprintf(String, "%02dm:%02ds", (((gSubMenuSelection + 1) * 5) / 60), (((gSubMenuSelection + 1) * 5) % 60));
            //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
            //ST7565_Gauge(4, 5, 179, gSubMenuSelection);
            gaugeLine = 4;
            gaugeMin = 5;
            gaugeMax = 179;
            //#endif
            break;

        #ifdef ENABLE_VOICE
            case MENU_VOICE:
                strcpy(String, SUBV(gSubMenu_VOICE[gSubMenuSelection], gSubMenu_VOICE_CN[gSubMenuSelection]));
                break;
        #endif

        case MENU_SC_REV:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, "STOP");
            }
            else if(gSubMenuSelection < 81)
            {
                sprintf(String, "CARRIER\n%02ds:%03dms", ((gSubMenuSelection * 250) / 1000), ((gSubMenuSelection * 250) % 1000));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(5, 1, 80, gSubMenuSelection);
                gaugeLine = 5;
                gaugeMin = 1;
                gaugeMax = 80;
                //#endif
            }
            else
            {
                sprintf(String, "TIMEOUT\n%02dm:%02ds", (((gSubMenuSelection - 80) * 5) / 60), (((gSubMenuSelection - 80) * 5) % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(5, 80, 104, gSubMenuSelection);
                gaugeLine = 5;
                gaugeMin = 80;
                gaugeMax = 104;
                //#endif
            }
            break;

        case MENU_MDF:
            strcpy(String, SUBV(gSubMenu_MDF[gSubMenuSelection], gSubMenu_MDF_CN[gSubMenuSelection]));
            break;

        case MENU_RP_STE:
            sprintf(String, gSubMenuSelection == 0 ? SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]) : "%u*100ms", gSubMenuSelection);
            break;

        case MENU_LIST_CH:
        case MENU_S_LIST:
            if (gSubMenuSelection == MR_CHANNELS_LIST + 1)
                strcpy(String, "ALL");
            else if (gSubMenuSelection == 0 && m == MENU_LIST_CH)
                strcpy(String, "OFF");
            else {
                const char *name = gListName[gSubMenuSelection - 1];
                
                // If first character is empty/invalid, display "N/A"
                if (IsEmptyName(name, sizeof(gListName[0])))
                    sprintf(String, "%02u", gSubMenuSelection);
                else
                    sprintf(String, "%02u (%.3s)", gSubMenuSelection, name);
            }
            break;
            
        #ifdef ENABLE_ALARM
            case MENU_AL_MOD:
                strcpy(String, SUBV(gSubMenu_AL_MOD[gSubMenuSelection], gSubMenu_AL_MOD_CN[gSubMenuSelection]));
                break;
        #endif

#ifdef ENABLE_DTMF_CALLING
        case MENU_ANI_ID:
            strcpy(String, gEeprom.ANI_DTMF_ID);
            break;
#endif
        case MENU_UPCODE:
            if (gEeprom.DTMF_UP_CODE[8] != '\0' && gEeprom.DTMF_UP_CODE[8] != 0xFF) {
                sprintf(String, "%.8s\n%.8s", gEeprom.DTMF_UP_CODE, gEeprom.DTMF_UP_CODE + 8);
            } else {
                sprintf(String, "%.8s", gEeprom.DTMF_UP_CODE);
            }
            break;

        case MENU_DWCODE:
            if (gEeprom.DTMF_DOWN_CODE[8] != '\0' && gEeprom.DTMF_DOWN_CODE[8] != 0xFF) {
                sprintf(String, "%.8s\n%.8s", gEeprom.DTMF_DOWN_CODE, gEeprom.DTMF_DOWN_CODE + 8);
            } else {
                sprintf(String, "%.8s", gEeprom.DTMF_DOWN_CODE);
            }
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_RSP:
            strcpy(String, SUBV(gSubMenu_D_RSP[gSubMenuSelection], gSubMenu_D_RSP_CN[gSubMenuSelection]));
            break;

        case MENU_D_HOLD:
            sprintf(String, "%ds", gSubMenuSelection);
            break;
#endif
        case MENU_D_PRE:
            sprintf(String, "%d*10ms", gSubMenuSelection);
            break;

        case MENU_PTT_ID:
            strcpy(String, SUBV(gSubMenu_PTT_ID[gSubMenuSelection], gSubMenu_PTT_ID_CN[gSubMenuSelection]));
            break;

        case MENU_BAT_TXT:
            strcpy(String, SUBV(gSubMenu_BAT_TXT[gSubMenuSelection], gSubMenu_BAT_TXT_CN[gSubMenuSelection]));
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_LIST:
            gIsDtmfContactValid = DTMF_GetContact((int)gSubMenuSelection - 1, Contact);
            if (!gIsDtmfContactValid)
                strcpy(String, "NULL");
            else
                memcpy(String, Contact, 8);
            break;
#endif

        case MENU_PONMSG:
            strcpy(String, SUBV(gSubMenu_PONMSG[gSubMenuSelection], gSubMenu_PONMSG_CN[gSubMenuSelection]));
            break;

        case MENU_ROGER:
            strcpy(String, SUBV(gSubMenu_ROGER[gSubMenuSelection], gSubMenu_ROGER_CN[gSubMenuSelection]));
            break;

        case MENU_VOL: {
            // SysInf is paginated. Pages appear in this order, only when their
            // feature flag is enabled:
            //   0          -> identity
            //   next       -> Build date/time         (ENABLE_FEAT_F4HWN)
            //   next       -> Battery                 (ENABLE_FEAT_F4HWN)
            //   next       -> Flash / SRAM usage      (ENABLE_FEAT_F4HWN_MEM)
            //   next, +1   -> CODE / WIKI QR codes    (ENABLE_FEAT_F4HWN_QRCODE)
            // In non-F4HWN builds, page 0 keeps the old battery-voltage display.
            const uint8_t page = (uint8_t)gSubMenuSelection;
            uint8_t       p    = 0;

            if (page == p++) {
                // Page 0: firmware identity.
#ifdef ENABLE_FEAT_F4HWN
                sprintf(String, "Mangosteen\n%s\nBD1AHN", VERSION_STRING_2);
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
                UI_PrintStringSmallNormal("BD1AHN", menu_item_x1 - 1, menu_item_x2, 6);
#endif
#else
                sprintf(String, "%u.%02uV\n%u%%",
                    gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                    BATTERY_VoltsToPercent(gBatteryVoltageAverage));
#endif
                break;
            }
#ifdef ENABLE_FEAT_F4HWN
            if (page == p++) {
                sprintf(String, "BUILD\n%s\n%s", BuildDate, BuildTime);
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
                strcpy(top_right_badge, "BUILD");
                UI_PrintStringSmallNormal(BuildDate, menu_item_x1 - 1, menu_item_x2, 3);
                UI_PrintStringSmallNormal(BuildTime, menu_item_x1 - 1, menu_item_x2, 4);
                UI_PrintStringSmallNormal(BuildCommit, menu_item_x1 - 1, menu_item_x2, 6);
                already_printed = true;
#endif
                break;
            }

            if (page == p++) {
                sprintf(String, "BATTERY\n%u.%02uV %u%%\n%s",
                    gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                    BATTERY_VoltsToPercent(gBatteryVoltageAverage),
                    gSubMenu_BATTYP[gEeprom.BATTERY_TYPE]);
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
                {
                    char val[16];

                    strcpy(top_right_badge, "BATTERY");
                    sprintf(val, "%u.%02uV %u%%",
                        gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                        BATTERY_VoltsToPercent(gBatteryVoltageAverage));
                    UI_PrintStringSmallNormal(val, menu_item_x1 - 1, menu_item_x2, 3);
                    UI_PrintStringSmallNormal(gSubMenu_BATTYP[gEeprom.BATTERY_TYPE], menu_item_x1 - 1, menu_item_x2, 5);
                    already_printed = true;
                }
#endif
                break;
            }
#endif
#ifdef ENABLE_FEAT_F4HWN_MEM
            if (page == p++) {
                uint16_t flash_pct = 0;
                uint16_t ram_pct   = 0;
                UI_GetMemPercents(&flash_pct, &ram_pct);

                sprintf(String, "MEMORY\nFLASH %u.%u%%\nSRAM %u.%u%%",
                        (unsigned)(flash_pct / 100), (unsigned)((flash_pct / 10) % 10),
                        (unsigned)(ram_pct / 100), (unsigned)((ram_pct / 10) % 10));
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
                {
                    char val[16];

                    strcpy(top_right_badge, "MEMORY");
                    sprintf(val, "FLASH %u.%u%%",
                            (unsigned)(flash_pct / 100), (unsigned)((flash_pct / 10) % 10));
                    UI_PrintStringSmallNormal(val, menu_item_x1 - 1, menu_item_x2, 3);
                    sprintf(val, "SRAM  %u.%u%%",
                            (unsigned)(ram_pct / 100), (unsigned)((ram_pct / 10) % 10));
                    UI_PrintStringSmallNormal(val, menu_item_x1 - 1, menu_item_x2, 5);
                    already_printed = true;
                }
#endif
                break;
            }
#endif
#ifdef ENABLE_FEAT_F4HWN_QRCODE
            // Right zone: x=49..127 (79 px). QR centered at x=72..104.
            // Capsule label above QR (small-font Inverse style at fb line 1).
            if (page == p || page == p + 1) {
                const bool is_wiki = (page == (p + 1));

                sprintf(String, "%s\nQR CODE", is_wiki ? "WIKI" : "CODE");
#ifndef ENABLE_CUSTOM_MENU_LAYOUT
                strcpy(top_right_badge, is_wiki ? "WIKI" : "CODE");
                UI_DrawQRCode(is_wiki, 72, 28);
                already_printed = true;
#endif
                break;
            }

            p += 2; 
#endif
            break;
        }

        case MENU_RESET:
            strcpy(String, SUBV(gSubMenu_RESET[gSubMenuSelection], gSubMenu_RESET_CN[gSubMenuSelection]));
            break;

        case MENU_F_LOCK:
#ifdef ENABLE_FEAT_F4HWN
            if(!gIsInSubMenu && gUnlockAllTxConfCnt>0 && gUnlockAllTxConfCnt<3)
#else
            if(!gIsInSubMenu && gUnlockAllTxConfCnt>0 && gUnlockAllTxConfCnt<10)
#endif
                strcpy(String, "READ\nMANUAL");
            else
                strcpy(String, gSubMenu_F_LOCK[gSubMenuSelection]);
            break;

        #ifdef ENABLE_F_CAL_MENU
            case MENU_F_CALI:
                {
                    const uint32_t value   = 22656 + gSubMenuSelection;
                    const uint32_t xtal_Hz = (0x4f0000u + value) * 5;

                    writeXtalFreqCal(gSubMenuSelection, false);

                    sprintf(String, "%d\n%u.%06u\nMHz",
                        gSubMenuSelection,
                        xtal_Hz / 1000000, xtal_Hz % 1000000);
                }
                break;
        #endif

        case MENU_BATCAL:
        {
            const uint16_t vol = (uint32_t)gBatteryVoltageAverage * gBatteryCalibration[3] / gSubMenuSelection;
            sprintf(String, "%u.%02uV\n%u", vol / 100, vol % 100, gSubMenuSelection);
            break;
        }

        case MENU_BATTYP:
            strcpy(String, gSubMenu_BATTYP[gSubMenuSelection]);
            break;

        case MENU_SET_NAV:
            strcpy(String, SUBV(gSubMenu_SET_NAV[gSubMenuSelection], gSubMenu_SET_NAV_CN[gSubMenuSelection]));
            break;

        case MENU_F1SHRT:
        case MENU_F1LONG:
        case MENU_F2SHRT:
        case MENU_F2LONG:
        case MENU_MLONG:
            strcpy(String, UI_MENU_GetSideFunctionName((uint8_t)gSubMenuSelection));
            break;

#ifdef ENABLE_FEAT_F4HWN_SLEEP
        case MENU_SET_OFF:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            }
            else if(gSubMenuSelection < 121)
            {
                sprintf(String, "%dh:%02dm", (gSubMenuSelection / 60), (gSubMenuSelection % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(4, 1, 120, gSubMenuSelection);
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 120;
                //#endif
            }
            break;
#endif

#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_PWR:
            sprintf(String, "%s\n%sW", SUBV(gSubMenu_TXP[gSubMenuSelection + 1], gSubMenu_TXP_CN[gSubMenuSelection + 1]), gSubMenu_SET_PWR[gSubMenuSelection]);
            break;
    
        case MENU_SET_PTT:
            strcpy(String, SUBV(gSubMenu_SET_PTT[gSubMenuSelection], gSubMenu_SET_PTT_CN[gSubMenuSelection]));
            break;

        case MENU_SET_TOT:
        case MENU_SET_EOT:
            strcpy(String, SUBV(gSubMenu_SET_TOT[gSubMenuSelection], gSubMenu_SET_TOT_CN[gSubMenuSelection])); // Same as SET_TOT
            break;

        case MENU_SET_CTR:
            #ifdef ENABLE_FEAT_F4HWN_CTR
                sprintf(String, "%d", gSubMenuSelection);
                gSetting_set_ctr = gSubMenuSelection;
                ST7565_ContrastAndInv();
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_SET_INV:
            #ifdef ENABLE_FEAT_F4HWN_INV
                strcpy(String, SUBV(gSubMenu_OFF_ON[gSubMenuSelection], gSubMenu_OFF_ON_CN[gSubMenuSelection]));
                ST7565_ContrastAndInv();
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_TX_LOCK:
            if(TX_freq_check(gEeprom.VfoInfo[gEeprom.TX_VFO].pTX->Frequency) == 0)
            {
                strcpy(String, "Inside\nF Lock\nPlan");
            }
            else
            {
                strcpy(String, SUBV(gSubMenu_OFF_ON[gSubMenuSelection], gSubMenu_OFF_ON_CN[gSubMenuSelection]));
            }
            break;

        case MENU_SET_LCK:
            strcpy(String, SUBV(gSubMenu_SET_LCK[gSubMenuSelection], gSubMenu_SET_LCK_CN[gSubMenuSelection]));
            break;

        case MENU_SET_MET:
        case MENU_SET_GUI:
            strcpy(String, SUBV(gSubMenu_SET_MET[gSubMenuSelection], gSubMenu_SET_MET_CN[gSubMenuSelection])); // Same as SET_MET
            break;

        #ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
            case MENU_SET_SCN:
                strcpy(String, SUBV(gSubMenu_SET_SCN[gSubMenuSelection], gSubMenu_SET_SCN_CN[gSubMenuSelection]));
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_AUDIO
            case MENU_SET_AUD:
                if(gTxVfo->Modulation == MODULATION_AM) {
                    strcpy(String, gSubMenu_SET_AUD_AM[gSubMenuSelection]);
                    strcpy(top_right_badge, "AM");
                }
                else if (gTxVfo->Modulation == MODULATION_USB) {
                    strcpy(String, "USB");
                    strcpy(top_right_badge, "USB");
                }
                else {
                    strcpy(String, gSubMenu_SET_AUD_FM[gSubMenuSelection]);
                    strcpy(top_right_badge, "FM");
                }
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            case MENU_SET_NFM:
                strcpy(String, SUBV(gSubMenu_SET_NFM[gSubMenuSelection], gSubMenu_SET_NFM_CN[gSubMenuSelection]));
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_VOL
            case MENU_SET_VOL:
                if(gSubMenuSelection == 0)
                {
                    strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
                }
                else if(gSubMenuSelection < 64)
                {
                    sprintf(String, "%02u", gSubMenuSelection);
                    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                    //ST7565_Gauge(4, 1, 63, gSubMenuSelection);
                    gaugeLine = 4;
                    gaugeMin = 1;
                    gaugeMax = 63;
                    //#endif
                }
                // gEeprom.VOLUME_GAIN = gSubMenuSelection;
                BK4819_SetRxAudioGain();
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            case MENU_SET_KEY:
                strcpy(String, SUBV(gSubMenu_SET_KEY[gSubMenuSelection], gSubMenu_SET_KEY_CN[gSubMenuSelection]));
                break;                
        #endif
#endif

    }

#ifdef ENABLE_CUSTOM_MENU_LAYOUT
    if (s_menu_fill_only)
        return;

    (void)already_printed;
    (void)gaugeLine;
    (void)gaugeMin;
    (void)gaugeMax;
    (void)menu_item_x1;
    (void)menu_item_x2;
    (void)top_right_badge;

    /* Side-panel draws from the switch are discarded; rebuild list + card. */
    UI_MENU_DrawListStyle(String);
    ST7565_BlitFullScreen();
    return;
#else
    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
    if(gaugeLine != 0)
    {
        ST7565_Gauge(gaugeLine, gaugeMin, gaugeMax, gSubMenuSelection);
    }
    //#endif

    if (!already_printed)
    {   // we now do multi-line text in a single string

        unsigned int y;
        unsigned int lines = 1;
        unsigned int len   = strlen(String);
        bool         small = false;

        if (String[0] != '\0')
        {
            // count number of lines
            for (i = 0; i < len; i++)
            {
                if (String[i] == '\n' && i < (len - 1))
                {   // found new line char
                    lines++;
                    String[i] = 0;  // null terminate the line
                }
            }

            if (lines > 3)
            {   // use small text
                small = true;
                if (lines > 7)
                    lines = 7;
            }

            y = (small ? 3 : 2) - (lines / 2); 

            // draw the text lines
            for (i = 0; i < len && lines > 0; lines--)
            {
                if (small)
                    UI_PrintStringSmallNormal(String + i, menu_item_x1, menu_item_x2, y);
                else
                    UI_PrintString(String + i, menu_item_x1, menu_item_x2, y, 8);

                // look for start of next line
                while (i < len && String[i] >= 32)
                    i++;

                // hop over the null term char(s)
                while (i < len && String[i] < 32)
                    i++;

                y += small ? 1 : 2;
            }
        }
    }

    if (m == MENU_S_PRI_CH_1 || m == MENU_S_PRI_CH_2)
    {

    }

    if ((m == MENU_R_CTCS || m == MENU_R_DCS) && gCssBackgroundScan)
        UI_PrintString("SCAN", menu_item_x1, menu_item_x2, 4, 8);

#ifdef ENABLE_DTMF_CALLING
    if (m == MENU_D_LIST && gIsDtmfContactValid) {
        Contact[11] = 0;
        memcpy(&gDTMF_ID, Contact + 8, 4);
        sprintf(String, "ID:%4s", gDTMF_ID);
        UI_PrintString(String, menu_item_x1, menu_item_x2, 4, 8);
    }
#endif

    const bool is_ctcs = (m == MENU_R_CTCS || m == MENU_T_CTCS);
    const bool is_dcs  = (m == MENU_R_DCS  || m == MENU_T_DCS);

    if (is_ctcs || is_dcs) {
        if (gSubMenuSelection == 0) {
            strcpy(top_right_badge, is_ctcs ? "00/00" : "000/00");
        } else {
            const uint8_t approved_index = is_ctcs ? 
                DCS_GetCtcssApprovedIndex(gSubMenuSelection - 1) : 
                DCS_GetDcsApprovedIndex(gSubMenuSelection - 1);
                
            const uint8_t width = is_ctcs ? 2 : 3;

            if (approved_index != 0xFF) {
                sprintf(top_right_badge, "%0*u/%02u", width, (unsigned)gSubMenuSelection, (unsigned)approved_index + 1);
            } else {
                sprintf(top_right_badge, "%0*u/--", width, (unsigned)gSubMenuSelection);
            }
        }
    }

#ifdef ENABLE_DTMF_CALLING
    if (m == MENU_D_LIST) {
        sprintf(top_right_badge, "%03d", gSubMenuSelection);
    }
#endif

    if (top_right_badge[0] != '\0') {
        UI_MENU_DrawTopRightRoundedBadge(top_right_badge, 1, true, menu_item_x1, menu_item_x2);
    }

    if ((m == MENU_RESET    ||
         m == MENU_MEM_CH   ||
         m == MENU_MEM_NAME ||
         m == MENU_DEL_CH) && gAskForConfirmation)
    {   // display confirmation
        char *pPrintStr = (gAskForConfirmation == 1) ? "SURE?" : "WAIT!";
        UI_PrintString(pPrintStr, menu_item_x1, menu_item_x2, 5, 8);
    }

    ST7565_BlitFullScreen();
#endif
}
