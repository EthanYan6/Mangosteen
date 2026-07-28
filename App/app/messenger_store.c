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
#include <string.h>
#include "app/messenger_store.h"
#include "app/messenger_packet.h"
#include "driver/py25q16.h"
#include "audio.h"
#include "misc.h"
#include "settings.h"

#define MSG_CFG_MAGIC 0x47u
#define MSG_CFG_VERSION 7u
#define MSG_CFG_FLASH_ADDR 0x012000u
#define MSG_CFG_FLASH_SIZE 0x1000u

MSG_Config_t gMessengerConfig;
MSG_Message_t gMessengerInbox[MSG_INBOX_CAPACITY];
MSG_Message_t gMessengerOutbox[MSG_OUTBOX_CAPACITY];

static const char * const kDraftsEn[MSG_DRAFT_CAPACITY] = {
    "OK", "NEED HELP", "WHERE ARE YOU?", "ON THE WAY",
    "ARRIVED SAFE", "CALL ME", "NEGATIVE", "BATTERY LOW"
};

/* GB2312 Chinese presets (5 used; rest stay empty). */
static const char * const kDraftsCn[] = {
    "\xD3\xD0\xC4\xDA\xB9\xED,\xD6\xD5\xD6\xB9\xBD\xBB\xD2\xD7", /* 有内鬼,终止交易 */
    "\xC7\xEB\xB5\xB3\xB7\xC5\xD0\xC4,\xC7\xBF\xB9\xFA\xD3\xD0\xCE\xD2", /* 请党放心,强国有我 */
    "\xC4\xE3\xBA\xC3", /* 你好 */
    "\xCA\xD5\xB5\xBD", /* 收到 */
    "73",
};

static void MSG_STORE_FillDefaultDrafts(void)
{
    memset(gMessengerConfig.drafts, 0, sizeof(gMessengerConfig.drafts));
    if (gUiLanguage == UI_LANGUAGE_CN) {
        for (uint8_t i = 0; i < (uint8_t)(sizeof(kDraftsCn) / sizeof(kDraftsCn[0])); i++)
            strncpy(gMessengerConfig.drafts[i], kDraftsCn[i], MSG_TEXT_LEN);
    } else {
        for (uint8_t i = 0; i < MSG_DRAFT_CAPACITY; i++)
            strncpy(gMessengerConfig.drafts[i], kDraftsEn[i], MSG_TEXT_LEN);
    }
}

void MSG_STORE_ApplyDefaultDrafts(void)
{
    MSG_STORE_Init();
    MSG_STORE_FillDefaultDrafts();
    MSG_STORE_SaveConfig();
}

const char *MSG_STORE_GetDraft(uint8_t index)
{
    if (index >= MSG_DRAFT_CAPACITY)
        return "";
    /* Storage is the single source of truth.  FillDefaultDrafts() pre-fills the
     * current language's presets on first boot and on every language switch, so
     * the built-in quick messages still appear until the user overwrites a slot
     * (save) or clears it (delete).  Reading storage directly is what makes a
     * saved draft visible in CN mode and a deleted slot show empty. */
    return gMessengerConfig.drafts[index];
}

static void MSG_STORE_DefaultConfig(void)
{
    memset(&gMessengerConfig, 0, sizeof(gMessengerConfig));
    gMessengerConfig.magic = MSG_CFG_MAGIC;
    gMessengerConfig.version = MSG_CFG_VERSION;
    gMessengerConfig.msg_rx = 1;
    gMessengerConfig.callsign_tx = 1;
    gMessengerConfig.msg_ack = 0;
    gMessengerConfig.msg_hop = 0;
    gMessengerConfig.msg_beep = 1;
    gMessengerConfig.msg_led = 1;
    gMessengerConfig.msg_debug = 0;
    gMessengerConfig.call_tone = 0;
    gMessengerConfig.call_vol = 1;
    gMessengerConfig.rng_rsp = 1;
    gMessengerConfig.next_msg_id = 1;
    strncpy(gMessengerConfig.callsign, "UVK1", MSG_CALLSIGN_EDIT_LEN);
    MSG_STORE_FillDefaultDrafts();
}

static void flash_read_struct(uint32_t addr, void *dst, uint16_t size)
{
    PY25Q16_ReadBuffer(addr, dst, size);
}

static void flash_write_struct(uint32_t addr, const void *src, uint16_t size)
{
    // Store Messenger data in a dedicated GOGUFW flash sector.
    // Do not use EEPROM-compatible addresses here: the old 0x1E80 location
    // overlaps the MR/channel memory compatibility area.
    if (size > MSG_CFG_FLASH_SIZE) return;

    /* Write-verify with retry: SPI flash writes can occasionally leave stale
     * bytes (e.g. when a sector erase is followed by a full-sector program).
     * We write, read back, and compare; if the read-back does not match we
     * retry up to 2 more times before giving up.  Interrupts are NOT disabled
     * here because PY25Q16_WriteBuffer/ReadBuffer rely on the DMA completion
     * interrupt (DMA1_Channel4_5_6_7_IRQn) to signal TC_Flag — masking IRQs
     * would deadlock in their `while (!TC_Flag)` spin loop. */
    uint8_t tmp[sizeof(gMessengerConfig)];
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        PY25Q16_WriteBuffer(addr, src, size, false);
        PY25Q16_ReadBuffer(addr, tmp, size);
        if (memcmp(src, tmp, size) == 0)
            return;
    }
}

static void MSG_STORE_SanitizeCallsign(void)
{
    gMessengerConfig.callsign[MSG_CALLSIGN_EDIT_LEN] = 0;
    for (uint8_t i = 0; i < MSG_CALLSIGN_EDIT_LEN; i++) {
        char c = gMessengerConfig.callsign[i];
        if (c == 0) break;
        if (c >= 'a' && c <= 'z') gMessengerConfig.callsign[i] = (char)(c - ('a' - 'A'));
    }
}

void MSG_STORE_SaveConfig(void)
{
    MSG_STORE_SanitizeCallsign();
    flash_write_struct(MSG_CFG_FLASH_ADDR, &gMessengerConfig, sizeof(gMessengerConfig));
}

static void MSG_STORE_CopyCommonFromLegacyV4(const MSG_Config_t *old)
{
    gMessengerConfig.msg_rx = old->msg_rx;
    gMessengerConfig.callsign_tx = old->callsign_tx;
    gMessengerConfig.msg_ack = old->msg_ack;
    gMessengerConfig.msg_hop = old->msg_hop;
    gMessengerConfig.msg_beep = old->msg_beep;
    gMessengerConfig.msg_led = old->msg_led;
    gMessengerConfig.msg_debug = old->msg_debug;
    gMessengerConfig.next_msg_id = old->next_msg_id ? old->next_msg_id : 1;
    memcpy(gMessengerConfig.callsign, old->callsign, sizeof(gMessengerConfig.callsign));
    memcpy(gMessengerConfig.drafts, old->drafts, sizeof(gMessengerConfig.drafts));
}

void MSG_STORE_Init(void)
{
    flash_read_struct(MSG_CFG_FLASH_ADDR, &gMessengerConfig, sizeof(gMessengerConfig));
    if (gMessengerConfig.magic != MSG_CFG_MAGIC) {
        MSG_STORE_DefaultConfig();
        MSG_STORE_SaveConfig();
    } else if (gMessengerConfig.version == 4u) {
        // Clean v0.3.12 migration: common fields keep their original offsets;
        // CllTon/CllVol are appended at the end only.
        MSG_Config_t old = gMessengerConfig;
        MSG_STORE_DefaultConfig();
        MSG_STORE_CopyCommonFromLegacyV4(&old);
        gMessengerConfig.call_tone = 0;
        gMessengerConfig.call_vol = 1;
        gMessengerConfig.rng_rsp = 1;
        MSG_STORE_SaveConfig();
    } else if (gMessengerConfig.version == 6u) {
        /* v7 appends RngRsp at the end only; preserve all v6 offsets. */
        gMessengerConfig.version = MSG_CFG_VERSION;
        if (gMessengerConfig.call_tone > 4u) gMessengerConfig.call_tone = 0;
        if (gMessengerConfig.call_vol > 1u) gMessengerConfig.call_vol = 1;
        gMessengerConfig.rng_rsp = 1;
        MSG_STORE_SanitizeCallsign();
        MSG_STORE_SaveConfig();
    } else if (gMessengerConfig.version != MSG_CFG_VERSION) {
        MSG_STORE_DefaultConfig();
        MSG_STORE_SaveConfig();
    } else {
        if (gMessengerConfig.call_tone > 4u) gMessengerConfig.call_tone = 0;
        if (gMessengerConfig.call_vol > 1u) gMessengerConfig.call_vol = 1;
        if (gMessengerConfig.rng_rsp > 1u) gMessengerConfig.rng_rsp = 1;
        MSG_STORE_SanitizeCallsign();
    }
}

static uint8_t count_list(MSG_Message_t *list, uint8_t cap)
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < cap; i++) if (list[i].used) n++;
    return n;
}

static void add_to_list(MSG_Message_t *list, uint8_t cap, const char *text, const char *from, const char *to,
                        uint16_t id, uint8_t ttl_init, uint8_t ttl_remain, bool unread)
{
    for (int i = cap - 1; i > 0; i--) list[i] = list[i - 1];
    memset(&list[0], 0, sizeof(list[0]));
    list[0].used = true;
    list[0].unread = unread;
    list[0].id = id;
    list[0].ttl_init = ttl_init;
    list[0].ttl_remain = ttl_remain;
    list[0].status = MSG_STATUS_NONE;
    list[0].age_seconds = 0u;
    strncpy(list[0].from, from && from[0] ? from : "UVK1", MSG_CALLSIGN_LEN);
    strncpy(list[0].to, to && to[0] ? to : "ALL", MSG_CALLSIGN_LEN);
    strncpy(list[0].text, text ? text : "TEST", MSG_TEXT_LEN);
}

uint16_t MSG_STORE_NextMsgId(void)
{
    uint16_t id = gMessengerConfig.next_msg_id++;
    if (gMessengerConfig.next_msg_id == 0) gMessengerConfig.next_msg_id = 1;
    MSG_STORE_SaveConfig();
    return id;
}

bool MSG_STORE_IsDuplicateInbox(const char *from, uint16_t id)
{
    if (id == 0u) return false;
    const char *safe_from = (from && from[0]) ? from : "UVK1";
    for (uint8_t i = 0; i < MSG_INBOX_CAPACITY; i++) {
        if (!gMessengerInbox[i].used) continue;
        if (gMessengerInbox[i].id == id &&
            strncmp(gMessengerInbox[i].from, safe_from, MSG_CALLSIGN_LEN) == 0) {
            return true;
        }
    }
    return false;
}

void MSG_STORE_AddInboxMessage(const char *text, const char *from, const char *to, uint16_t id, uint8_t ttl_init, uint8_t ttl_remain, bool unread)
{
    /* Duplicate retry frames must not create a second inbox entry or trigger
     * unread/beep state. ACK resend is handled by the RF layer before this call. */
    if (MSG_STORE_IsDuplicateInbox(from, id)) return;

    add_to_list(gMessengerInbox, MSG_INBOX_CAPACITY, text, from, to, id, ttl_init, ttl_remain, unread);
    gUpdateStatus = true;
    gUpdateDisplay = true;
    if (unread && gMessengerConfig.msg_beep) {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_FORCE;
    }
}

void MSG_STORE_AddOutboxMessage(const char *text, const char *from, const char *to, uint16_t id, uint8_t ttl_init, uint8_t ttl_remain)
{
    add_to_list(gMessengerOutbox, MSG_OUTBOX_CAPACITY, text, from, to, id, ttl_init, ttl_remain, false);
    gMessengerOutbox[0].status = MSG_STATUS_PENDING;
}

void MSG_STORE_SetOutboxStatusById(uint16_t id, uint8_t status)
{
    for (uint8_t i = 0; i < MSG_OUTBOX_CAPACITY; i++) {
        if (gMessengerOutbox[i].used && gMessengerOutbox[i].id == id) {
            gMessengerOutbox[i].status = status;
            gUpdateDisplay = true;
            return;
        }
    }
}

void MSG_STORE_AddOutboxAckSourceById(uint16_t id, const char *from)
{
    if (!from || !from[0]) return;
    for (uint8_t i = 0; i < MSG_OUTBOX_CAPACITY; i++) {
        MSG_Message_t *m = &gMessengerOutbox[i];
        if (!m->used || m->id != id) continue;

        for (uint8_t j = 0; j < m->ack_count && j < MSG_ACK_SOURCE_MAX; j++) {
            if (strncmp(m->ack_from[j], from, MSG_ACK_ID_LEN) == 0) {
                return;
            }
        }

        if (m->ack_count < MSG_ACK_SOURCE_MAX) {
            memset(m->ack_from[m->ack_count], 0, sizeof(m->ack_from[m->ack_count]));
            strncpy(m->ack_from[m->ack_count], from, MSG_ACK_ID_LEN);
            m->ack_count++;
            gUpdateDisplay = true;
        }
        return;
    }
}

void MSG_STORE_AddInboxDemo(const char *text)
{
    MSG_STORE_AddInboxMessage(text, "DEMO", "ALL", MSG_STORE_NextMsgId(), gMessengerConfig.msg_hop, gMessengerConfig.msg_hop, true);
}

void MSG_STORE_AddOutboxDemo(const char *text)
{
    MSG_STORE_AddOutboxMessage(text, gMessengerConfig.callsign, "ALL", MSG_STORE_NextMsgId(), gMessengerConfig.msg_hop, gMessengerConfig.msg_hop);
}

bool MSG_STORE_InjectNativePacket(const char *text)
{
    uint8_t frame[MSG_PKT_WIRE_LEN];
    MSG_Packet_t pkt;
    uint16_t id = MSG_STORE_NextMsgId();
    if (!MSG_PACKET_BuildText(frame, sizeof(frame), id, "NODE2", text ? text : "NATIVE TEST", gMessengerConfig.msg_hop)) return false;
    if (!MSG_PACKET_Parse(frame, sizeof(frame), &pkt)) return false;
    if (pkt.type != MSG_PKT_TYPE_TEXT) return false;
    MSG_STORE_AddInboxMessage(pkt.payload, pkt.from, pkt.to, pkt.id, pkt.ttl_init, pkt.ttl_remain, true);
    return true;
}

static void delete_from_list(MSG_Message_t *list, uint8_t cap, uint8_t index)
{
    if (index >= cap || !list[index].used) return;
    for (uint8_t i = index; i + 1 < cap; i++) list[i] = list[i + 1];
    memset(&list[cap - 1], 0, sizeof(list[cap - 1]));
}

void MSG_STORE_DeleteInbox(uint8_t index) { delete_from_list(gMessengerInbox, MSG_INBOX_CAPACITY, index); }
void MSG_STORE_DeleteOutbox(uint8_t index) { delete_from_list(gMessengerOutbox, MSG_OUTBOX_CAPACITY, index); }
void MSG_STORE_MarkInboxRead(uint8_t index)
{
    if (index < MSG_INBOX_CAPACITY) {
        gMessengerInbox[index].unread = false;
        gUpdateStatus = true;
        gUpdateDisplay = true;
    }
}
uint8_t MSG_STORE_CountInbox(void) { return count_list(gMessengerInbox, MSG_INBOX_CAPACITY); }
uint8_t MSG_STORE_CountOutbox(void) { return count_list(gMessengerOutbox, MSG_OUTBOX_CAPACITY); }
uint8_t MSG_STORE_CountDrafts(void) { return MSG_DRAFT_CAPACITY; }

void MSG_STORE_SetDraft(uint8_t index, const char *text)
{
    if (index >= MSG_DRAFT_CAPACITY) return;
    memset(gMessengerConfig.drafts[index], 0, sizeof(gMessengerConfig.drafts[index]));
    if (text && text[0]) {
        strncpy(gMessengerConfig.drafts[index], text, MSG_TEXT_LEN);
        gMessengerConfig.drafts[index][MSG_TEXT_LEN] = 0;
    }
    MSG_STORE_SaveConfig();
    gUpdateDisplay = true;
}


bool MSG_STORE_HasUnread(void)
{
    for (uint8_t i = 0; i < MSG_INBOX_CAPACITY; i++) if (gMessengerInbox[i].used && gMessengerInbox[i].unread) return true;
    return false;
}
