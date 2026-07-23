#ifndef APP_MESSENGER_T9_H
#define APP_MESSENGER_T9_H
#include <stdbool.h>
#include <stdint.h>
#include "driver/keyboard.h"

typedef struct {
    char *buffer;
    uint8_t max_len;
    uint8_t len;
    bool upper;
    uint8_t mode; /* 0=upper(B), 1=lower(b), 2=numeric(2), 3=pinyin */
    KEY_Code_t pending_key;
    uint8_t cycle_index;
    uint16_t pending_ticks;
    bool has_pending;
} MSG_T9Editor_t;

void MSG_T9_Start(MSG_T9Editor_t *ed, char *buf, uint8_t max_len);
bool MSG_T9_HandleKey(MSG_T9Editor_t *ed, KEY_Code_t key);
bool MSG_T9_HandleLongKey(MSG_T9Editor_t *ed, KEY_Code_t key);
void MSG_T9_Tick(MSG_T9Editor_t *ed);
void MSG_T9_Commit(MSG_T9Editor_t *ed);

#endif
