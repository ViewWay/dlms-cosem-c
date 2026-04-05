/**
 * @file dlms_hdlc.c
 * @brief HDLC framing for DLMS/COSEM
 */
#include "dlms/dlms_hdlc.h"
#include <string.h>
#include <stdio.h>

/* ===== CRC-16 CCITT HDLC (lookup table) ===== */
static uint16_t crc16_table[256];

static void crc16_init_table(void) {
    static bool initialized = false;
    if (initialized) return;
    for (uint16_t i = 0; i < 256; i++) {
        uint16_t crc = 0;
        uint16_t c = i << 8;
        for (int j = 0; j < 8; j++) {
            if ((crc ^ c) & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc = (uint16_t)(crc << 1);
            }
            c = (uint16_t)(c << 1);
        }
        crc16_table[i] = crc;
    }
    initialized = true;
}

static uint16_t crc16_compute(const uint8_t *data, size_t len, uint16_t initial_crc) {
    crc16_init_table();
    uint16_t crc = initial_crc;
    for (size_t i = 0; i < len; i++) {
        uint8_t b = dlms_reverse_bits8(data[i]);
        uint8_t idx = (uint8_t)(((crc >> 8) & 0xFF) ^ b);
        crc = (uint16_t)(((crc << 8) & 0xFF00) ^ crc16_table[idx]);
    }
    return crc;
}

static uint16_t crc16_finalize(uint16_t crc) {
    /* Reverse and XOR */
    uint8_t lsb = dlms_reverse_bits8((uint8_t)(crc & 0xFF)) ^ 0xFF;
    uint8_t msb = dlms_reverse_bits8((uint8_t)((crc >> 8) & 0xFF)) ^ 0xFF;
    return ((uint16_t)msb << 8) | lsb;
}

uint16_t dlms_crc16(const uint8_t *data, size_t len) {
    return crc16_finalize(crc16_compute(data, len, 0xFFFF));
}

uint16_t dlms_crc16_update(uint16_t crc, const uint8_t *data, size_t len) {
    /* For incremental updates, reverse the finalization before continuing */
    uint8_t lsb = dlms_reverse_bits8((uint8_t)(crc & 0xFF)) ^ 0xFF;
    uint8_t msb = dlms_reverse_bits8((uint8_t)((crc >> 8) & 0xFF)) ^ 0xFF;
    uint16_t unfinalized = ((uint16_t)msb << 8) | lsb;
    return crc16_finalize(crc16_compute(data, len, unfinalized));
}

/* ===== HDLC byte stuffing ===== */

static bool needs_escape(uint8_t b) {
    return (b == DLMS_HDLC_FLAG || b == DLMS_HDLC_ESCAPE || b == 0x5D || b == 0x7D);
}

size_t dlms_hdlc_stuff(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap) {
    size_t out = 0;
    for (size_t i = 0; i < len; i++) {
        if (needs_escape(src[i])) {
            if (out + 2 > dst_cap) return 0;
            dst[out++] = DLMS_HDLC_ESCAPE;
            dst[out++] = src[i] ^ 0x20;
        } else {
            if (out + 1 > dst_cap) return 0;
            dst[out++] = src[i];
        }
    }
    return out;
}

size_t dlms_hdlc_unstuff(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap) {
    size_t out = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == DLMS_HDLC_ESCAPE) {
            if (i + 1 >= len) return 0;
            i++;
            if (out >= dst_cap) return 0;
            dst[out++] = src[i] ^ 0x20;
        } else {
            if (out >= dst_cap) return 0;
            dst[out++] = src[i];
        }
    }
    return out;
}

/* ===== HDLC Address ===== */

int dlms_hdlc_address_encode(const dlms_hdlc_address_t *addr, bool is_client,
                             uint8_t *dst, size_t dst_size, size_t *written) {
    if (!addr || !dst || !written) return DLMS_ERROR_INVALID;
    *written = 0;
    if (is_client) {
        if (dst_size < 1) return DLMS_ERROR_BUFFER;
        dst[0] = (uint8_t)((addr->logical_address << 1) | 0x01);
        *written = 1;
    } else {
        if (addr->has_physical) {
            if (dst_size < 4) return DLMS_ERROR_BUFFER;
            uint16_t la = addr->logical_address;
            uint16_t pa = addr->physical_address;
            dst[0] = (uint8_t)((la >> 6) & 0xFF);
            dst[1] = (uint8_t)((la & 0x7F) << 1);
            dst[2] = (uint8_t)((pa >> 6) & 0xFF);
            dst[3] = (uint8_t)(((pa & 0x7F) << 1) | 0x01);
            *written = 4;
        } else {
            if (dst_size < 2) return DLMS_ERROR_BUFFER;
            uint16_t la = addr->logical_address;
            dst[0] = (uint8_t)((la >> 6) & 0xFF);
            dst[1] = (uint8_t)(((la & 0x7F) << 1) | 0x01);
            *written = 2;
        }
    }
    return DLMS_OK;
}

int dlms_hdlc_address_decode(const uint8_t *data, size_t len, bool is_client,
                             dlms_hdlc_address_t *addr, size_t *consumed) {
    if (!data || !addr || !consumed || len < 1) return DLMS_ERROR_INVALID;
    *consumed = 0;
    if (is_client) {
        addr->logical_address = data[0] >> 1;
        addr->physical_address = 0;
        addr->has_physical = false;
        *consumed = 1;
    } else {
        /* Find end of address (LSB=1) */
        size_t pos = 0;
        while (pos < len && pos < 4) {
            pos++;
            if (data[pos - 1] & 0x01) break;
        }
        if (pos == 4 && !(data[3] & 0x01)) return DLMS_ERROR_PARSE;
        if (pos == 1) {
            addr->logical_address = data[0] >> 1;
            addr->has_physical = false;
            addr->physical_address = 0;
        } else if (pos == 2) {
            addr->logical_address = (data[0] >> 1) | ((uint16_t)(data[1] & 0xFE) << 6);
            addr->has_physical = false;
            addr->physical_address = 0;
        } else {
            addr->logical_address = (data[0] >> 1) | ((uint16_t)(data[1] & 0xFE) << 6);
            addr->physical_address = (data[2] >> 1) | ((uint16_t)(data[3] & 0xFE) << 6);
            addr->has_physical = true;
        }
        *consumed = pos;
    }
    addr->extended = (*consumed > 2);
    return DLMS_OK;
}

/* ===== Frame Format ===== */

int dlms_hdlc_format_encode(const dlms_hdlc_format_t *fmt, uint8_t *dst) {
    if (!fmt || !dst) return DLMS_ERROR_INVALID;
    uint16_t val = 0xA000 | (fmt->length & 0x7FF);
    if (fmt->segmented) val |= 0x0800;
    dst[0] = (uint8_t)(val >> 8);
    dst[1] = (uint8_t)(val & 0xFF);
    return DLMS_OK;
}

int dlms_hdlc_format_decode(const uint8_t *data, dlms_hdlc_format_t *fmt) {
    if (!data || !fmt) return DLMS_ERROR_INVALID;
    uint16_t val = ((uint16_t)data[0] << 8) | data[1];
    if ((val & 0xF000) != 0xA000) return DLMS_ERROR_PARSE;
    fmt->length = val & 0x7FF;
    fmt->segmented = (val & 0x0800) != 0;
    return DLMS_OK;
}

/* ===== Control Field ===== */

int dlms_hdlc_control_encode_snrm(uint8_t *dst) {
    if (!dst) return DLMS_ERROR_INVALID;
    dst[0] = 0x93; /* SNRM with Final bit */
    return DLMS_OK;
}

int dlms_hdlc_control_encode_ua(uint8_t *dst) {
    if (!dst) return DLMS_ERROR_INVALID;
    dst[0] = 0x73; /* UA with Final bit */
    return DLMS_OK;
}

int dlms_hdlc_control_encode_rr(uint8_t recv_seq, uint8_t *dst) {
    if (!dst || recv_seq > 7) return DLMS_ERROR_INVALID;
    dst[0] = 0x01 | (recv_seq << 5) | 0x10;
    return DLMS_OK;
}

int dlms_hdlc_control_encode_info(uint8_t send_seq, uint8_t recv_seq, bool final, uint8_t *dst) {
    if (!dst || send_seq > 7 || recv_seq > 7) return DLMS_ERROR_INVALID;
    dst[0] = (send_seq << 1) | (recv_seq << 5) | (final ? 0x10 : 0x00);
    return DLMS_OK;
}

int dlms_hdlc_control_encode_disc(uint8_t *dst) {
    if (!dst) return DLMS_ERROR_INVALID;
    dst[0] = 0x53;
    return DLMS_OK;
}

int dlms_hdlc_control_encode_ui(bool final, uint8_t *dst) {
    if (!dst) return DLMS_ERROR_INVALID;
    dst[0] = 0x03 | (final ? 0x10 : 0x00);
    return DLMS_OK;
}

int dlms_hdlc_control_decode(uint8_t byte, dlms_hdlc_frame_type_t type,
                             dlms_hdlc_control_t *control) {
    if (!control) return DLMS_ERROR_INVALID;
    control->is_final = (byte & 0x10) != 0;
    switch (type) {
        case DLMS_HDLC_FRAME_INFO:
            control->send_seq = (byte >> 1) & 0x07;
            control->recv_seq = (byte >> 5) & 0x07;
            break;
        case DLMS_HDLC_FRAME_RR:
            control->send_seq = 0;
            control->recv_seq = (byte >> 5) & 0x07;
            break;
        default:
            control->send_seq = 0;
            control->recv_seq = 0;
    }
    return DLMS_OK;
}

/* ===== HDLC Parameters ===== */

int dlms_hdlc_param_list_init(dlms_hdlc_param_list_t *list) {
    if (!list) return DLMS_ERROR_INVALID;
    memset(list, 0, sizeof(*list));
    return DLMS_OK;
}

int dlms_hdlc_param_list_set(dlms_hdlc_param_list_t *list, dlms_hdlc_param_type_t type, uint16_t value) {
    if (!list) return DLMS_ERROR_INVALID;
    for (uint8_t i = 0; i < list->count; i++) {
        if (list->params[i].type == type) {
            list->params[i].value = value;
            return DLMS_OK;
        }
    }
    if (list->count >= DLMS_HDLC_MAX_PARAMS) return DLMS_ERROR_BUFFER;
    list->params[list->count].type = type;
    list->params[list->count].value = value;
    list->count++;
    return DLMS_OK;
}

int dlms_hdlc_param_list_get(const dlms_hdlc_param_list_t *list, dlms_hdlc_param_type_t type, uint16_t *value) {
    if (!list || !value) return DLMS_ERROR_INVALID;
    for (uint8_t i = 0; i < list->count; i++) {
        if (list->params[i].type == type) {
            *value = list->params[i].value;
            return DLMS_OK;
        }
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_hdlc_param_list_encode(const dlms_hdlc_param_list_t *list, uint8_t *dst, size_t dst_size, size_t *written) {
    if (!list || !dst || !written) return DLMS_ERROR_INVALID;
    *written = 0;
    for (uint8_t i = 0; i < list->count; i++) {
        const dlms_hdlc_param_t *p = &list->params[i];
        uint8_t vlen = 1;
        if (p->type == DLMS_PARAM_MAX_INFO_TX || p->type == DLMS_PARAM_MAX_INFO_RX ||
            p->type == DLMS_PARAM_INACTIVITY_TIMEOUT) vlen = 2;
        if (*written + 2 + vlen > dst_size) return DLMS_ERROR_BUFFER;
        dst[(*written)++] = (uint8_t)p->type;
        dst[(*written)++] = vlen;
        if (vlen == 2) {
            dst[(*written)++] = (uint8_t)(p->value >> 8);
        }
        dst[(*written)++] = (uint8_t)(p->value & 0xFF);
    }
    return DLMS_OK;
}

int dlms_hdlc_param_list_decode(const uint8_t *data, size_t len, dlms_hdlc_param_list_t *list) {
    if (!data || !list) return DLMS_ERROR_INVALID;
    dlms_hdlc_param_list_init(list);
    size_t pos = 0;
    while (pos + 2 <= len) {
        dlms_hdlc_param_type_t type = (dlms_hdlc_param_type_t)data[pos];
        uint8_t vlen = data[pos + 1];
        if (pos + 2 + vlen > len) return DLMS_ERROR_PARSE;
        uint16_t val = 0;
        for (uint8_t j = 0; j < vlen; j++) {
            val = (val << 8) | data[pos + 2 + j];
        }
        dlms_hdlc_param_list_set(list, type, val);
        pos += 2 + vlen;
    }
    return DLMS_OK;
}

/* ===== Internal: Determine frame type from control byte ===== */

static dlms_hdlc_frame_type_t detect_frame_type(uint8_t ctrl) {
    if ((ctrl & 0x01) == 0) return DLMS_HDLC_FRAME_INFO;
    if (ctrl == 0x93 || ctrl == 0x83) return DLMS_HDLC_FRAME_SNRM;
    if (ctrl == 0x73 || ctrl == 0x63) return DLMS_HDLC_FRAME_UA;
    if ((ctrl & 0x03) == 0x01) return DLMS_HDLC_FRAME_RR;
    if (ctrl == 0x53 || ctrl == 0x43) return DLMS_HDLC_FRAME_DISC;
    if ((ctrl & 0x03) == 0x03) return DLMS_HDLC_FRAME_UI;
    return DLMS_HDLC_FRAME_UNKNOWN;
}

/* ===== Frame decode ===== */

int dlms_hdlc_frame_decode(const uint8_t *data, size_t len, dlms_hdlc_frame_t *frame) {
    if (!data || !frame || len < 8) return DLMS_ERROR_INVALID;

    /* Unstuff the frame (between flags) */
    uint8_t unstuff_buf[DLMS_HDLC_MAX_FRAME_SIZE];
    /* Skip leading and trailing 0x7E */
    if (data[0] != DLMS_HDLC_FLAG || data[len - 1] != DLMS_HDLC_FLAG)
        return DLMS_ERROR_PARSE;

    size_t inner_len = len - 2;
    size_t unstuffed = dlms_hdlc_unstuff(data + 1, inner_len, unstuff_buf, sizeof(unstuff_buf));
    if (unstuffed == 0) return DLMS_ERROR_PARSE;

    /* Verify FCS */
    if (unstuffed < 3) return DLMS_ERROR_PARSE;
    uint16_t recv_fcs = ((uint16_t)unstuff_buf[unstuffed - 2] << 8) | unstuff_buf[unstuffed - 1];
    uint16_t calc_fcs = dlms_crc16(unstuff_buf, unstuffed - 2);
    if (recv_fcs != calc_fcs) return DLMS_ERROR_PARSE;

    /* Parse format field */
    dlms_hdlc_format_t fmt;
    int ret = dlms_hdlc_format_decode(unstuff_buf, &fmt);
    if (ret != DLMS_OK) return ret;

    if ((size_t)(fmt.length + 2) != unstuffed) return DLMS_ERROR_PARSE;

    memset(frame, 0, sizeof(*frame));

    /* Parse destination address (client) */
    size_t pos = 2;
    size_t consumed = 0;
    ret = dlms_hdlc_address_decode(unstuff_buf + pos, unstuffed - pos, true,
                                   &frame->dest, &consumed);
    if (ret != DLMS_OK) return ret;
    pos += consumed;

    /* Parse source address (server) */
    ret = dlms_hdlc_address_decode(unstuff_buf + pos, unstuffed - pos, false,
                                   &frame->src, &consumed);
    if (ret != DLMS_OK) return ret;
    pos += consumed;

    /* Control byte */
    uint8_t ctrl = unstuff_buf[pos++];

    /* HCS (if information field present) */
    frame->hcs = 0;
    frame->info = NULL;
    frame->info_len = 0;

    frame->type = detect_frame_type(ctrl);
    dlms_hdlc_control_decode(ctrl, frame->type, &frame->control);

    /* For frames with info: format+addr+ctrl+hcs(2) before info */
    if (frame->type == DLMS_HDLC_FRAME_INFO || frame->type == DLMS_HDLC_FRAME_UI ||
        frame->type == DLMS_HDLC_FRAME_SNRM || frame->type == DLMS_HDLC_FRAME_UA) {
        if (pos + 2 <= unstuffed - 2) {
            frame->hcs = ((uint16_t)unstuff_buf[pos] << 8) | unstuff_buf[pos + 1];
            pos += 2;
        }
        size_t info_len = unstuffed - 2 - pos;
        if (info_len > 0) {
            frame->info = unstuff_buf + pos;
            frame->info_len = (uint16_t)info_len;
        }
    }

    return DLMS_OK;
}

/* ===== Frame encode ===== */

int dlms_hdlc_frame_encode(uint8_t *dst, size_t dst_size, const dlms_hdlc_frame_t *frame) {
    if (!dst || !frame) return DLMS_ERROR_INVALID;

    uint8_t raw[DLMS_HDLC_MAX_FRAME_SIZE];
    size_t raw_len = 0;

    /* Destination address */
    size_t w = 0;
    int ret = dlms_hdlc_address_encode(&frame->dest, true, raw + 2, sizeof(raw) - 2, &w);
    if (ret != DLMS_OK) return ret;

    /* Source address */
    size_t sw = 0;
    ret = dlms_hdlc_address_encode(&frame->src, false, raw + 2 + w, sizeof(raw) - 2 - w, &sw);
    if (ret != DLMS_OK) return ret;

    /* Control byte */
    uint8_t ctrl = 0;
    switch (frame->type) {
        case DLMS_HDLC_FRAME_SNRM: dlms_hdlc_control_encode_snrm(&ctrl); break;
        case DLMS_HDLC_FRAME_UA: dlms_hdlc_control_encode_ua(&ctrl); break;
        case DLMS_HDLC_FRAME_RR: dlms_hdlc_control_encode_rr(frame->control.recv_seq, &ctrl); break;
        case DLMS_HDLC_FRAME_INFO:
            dlms_hdlc_control_encode_info(frame->control.send_seq, frame->control.recv_seq,
                                          frame->control.is_final, &ctrl); break;
        case DLMS_HDLC_FRAME_DISC: dlms_hdlc_control_encode_disc(&ctrl); break;
        case DLMS_HDLC_FRAME_UI:
            dlms_hdlc_control_encode_ui(frame->control.is_final, &ctrl); break;
        default: return DLMS_ERROR_NOT_SUPPORTED;
    }

    /* Calculate total frame length (without flags and FCS) */
    bool has_info = (frame->info != NULL && frame->info_len > 0);
    bool has_hcs = has_info || frame->type == DLMS_HDLC_FRAME_SNRM || frame->type == DLMS_HDLC_FRAME_UA;
    uint16_t frame_len = 2 + w + sw + 1 + (has_hcs ? 2 : 0) + frame->info_len;

    /* Format field */
    dlms_hdlc_format_t fmt = { .length = frame_len, .segmented = false };
    dlms_hdlc_format_encode(&fmt, raw);
    raw_len = 2;

    /* Addresses */
    memcpy(raw + raw_len, raw + 2, w + sw);
    raw_len += w + sw;

    /* Control */
    raw[raw_len++] = ctrl;

    /* HCS */
    if (has_hcs) {
        uint16_t hcs = dlms_crc16(raw, raw_len);
        raw[raw_len++] = (uint8_t)(hcs >> 8);
        raw[raw_len++] = (uint8_t)(hcs & 0xFF);
    }

    /* Information */
    if (has_info) {
        if (raw_len + frame->info_len > sizeof(raw)) return DLMS_ERROR_BUFFER;
        memcpy(raw + raw_len, frame->info, frame->info_len);
        raw_len += frame->info_len;
    }

    /* FCS */
    uint16_t fcs = dlms_crc16(raw, raw_len);
    raw[raw_len++] = (uint8_t)(fcs >> 8);
    raw[raw_len++] = (uint8_t)(fcs & 0xFF);

    /* Byte stuffing and add flags */
    if (dst_size < raw_len * 2 + 2) return DLMS_ERROR_BUFFER;
    dst[0] = DLMS_HDLC_FLAG;
    size_t stuffed = dlms_hdlc_stuff(raw, raw_len, dst + 1, dst_size - 2);
    if (stuffed == 0) return DLMS_ERROR_BUFFER;
    dst[1 + stuffed] = DLMS_HDLC_FLAG;
    return (int)(stuffed + 2);
}

/* ===== Streaming parser ===== */

dlms_hdlc_parser_t *dlms_hdlc_parser_create(uint8_t *buf, size_t buf_size) {
    static dlms_hdlc_parser_t parser;
    parser.buf = buf;
    parser.buf_cap = buf_size;
    parser.buf_len = 0;
    parser.state = HDLC_STATE_IDLE;
    return &parser;
}

void dlms_hdlc_parser_reset(dlms_hdlc_parser_t *parser) {
    if (parser) {
        parser->buf_len = 0;
        parser->state = HDLC_STATE_IDLE;
    }
}

int dlms_hdlc_parse(dlms_hdlc_parser_t *parser, const uint8_t *data, size_t len,
                    dlms_hdlc_frame_callback_t callback, void *user_data) {
    if (!parser || !data) return DLMS_ERROR_INVALID;

    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        switch (parser->state) {
            case HDLC_STATE_IDLE:
                if (b == DLMS_HDLC_FLAG) {
                    parser->state = HDLC_STATE_RECEIVING;
                    parser->buf_len = 0;
                }
                break;
            case HDLC_STATE_RECEIVING:
                if (b == DLMS_HDLC_FLAG) {
                    /* End of frame */
                    if (parser->buf_len > 0 && callback) {
                        dlms_hdlc_frame_t frame;
                        int ret = dlms_hdlc_frame_decode(
                            parser->buf, parser->buf_len + 2, &frame);
                        /* We need to wrap: pre+0x7E + buf + 0x7E */
                        if (ret == DLMS_OK) {
                            callback(&frame, user_data);
                        }
                    }
                    parser->buf_len = 0;
                    parser->state = HDLC_STATE_RECEIVING; /* stay for next */
                } else if (b == DLMS_HDLC_ESCAPE) {
                    parser->state = HDLC_STATE_ESCAPE;
                } else {
                    if (parser->buf_len < parser->buf_cap) {
                        parser->buf[parser->buf_len++] = b;
                    }
                }
                break;
            case HDLC_STATE_ESCAPE:
                if (parser->buf_len < parser->buf_cap) {
                    parser->buf[parser->buf_len++] = b ^ 0x20;
                }
                parser->state = HDLC_STATE_RECEIVING;
                break;
        }
    }
    return DLMS_OK;
}

/* ===== Frame builder helpers ===== */

int dlms_hdlc_build_snrm(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                         const dlms_hdlc_address_t *src, const dlms_hdlc_param_list_t *params) {
    dlms_hdlc_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = DLMS_HDLC_FRAME_SNRM;
    frame.dest = *dest;
    frame.src = *src;

    uint8_t info_buf[32];
    size_t info_len = 0;
    if (params && params->count > 0) {
        int ret = dlms_hdlc_param_list_encode(params, info_buf, sizeof(info_buf), &info_len);
        if (ret != DLMS_OK) return ret;
    }
    frame.info = (info_len > 0) ? info_buf : NULL;
    frame.info_len = (uint16_t)info_len;
    return dlms_hdlc_frame_encode(dst, dst_size, &frame);
}

int dlms_hdlc_build_ua(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                       const dlms_hdlc_address_t *src, const dlms_hdlc_param_list_t *params) {
    dlms_hdlc_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = DLMS_HDLC_FRAME_UA;
    frame.dest = *dest;
    frame.src = *src;

    uint8_t info_buf[32];
    size_t info_len = 0;
    if (params && params->count > 0) {
        int ret = dlms_hdlc_param_list_encode(params, info_buf, sizeof(info_buf), &info_len);
        if (ret != DLMS_OK) return ret;
    }
    frame.info = (info_len > 0) ? info_buf : NULL;
    frame.info_len = (uint16_t)info_len;
    return dlms_hdlc_frame_encode(dst, dst_size, &frame);
}

int dlms_hdlc_build_rr(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                       const dlms_hdlc_address_t *src, uint8_t recv_seq) {
    dlms_hdlc_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = DLMS_HDLC_FRAME_RR;
    frame.dest = *dest;
    frame.src = *src;
    frame.control.recv_seq = recv_seq;
    return dlms_hdlc_frame_encode(dst, dst_size, &frame);
}

int dlms_hdlc_build_info(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                         const dlms_hdlc_address_t *src, uint8_t send_seq, uint8_t recv_seq,
                         bool final, const uint8_t *info, uint16_t info_len) {
    dlms_hdlc_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = DLMS_HDLC_FRAME_INFO;
    frame.dest = *dest;
    frame.src = *src;
    frame.control.send_seq = send_seq;
    frame.control.recv_seq = recv_seq;
    frame.control.is_final = final;
    frame.info = info;
    frame.info_len = info_len;
    return dlms_hdlc_frame_encode(dst, dst_size, &frame);
}

int dlms_hdlc_build_disc(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                         const dlms_hdlc_address_t *src) {
    dlms_hdlc_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = DLMS_HDLC_FRAME_DISC;
    frame.dest = *dest;
    frame.src = *src;
    return dlms_hdlc_frame_encode(dst, dst_size, &frame);
}
