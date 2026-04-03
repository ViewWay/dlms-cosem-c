/**
 * @file dlms_axdr.c
 * @brief A-XDR encoding/decoding for DLMS/COSEM
 */
#include "dlms/dlms_axdr.h"
#include <string.h>

/* ===== Internal helpers ===== */

static int encode_length_prefix(uint32_t len, uint8_t *dst, size_t cap, size_t *w) {
    if (len <= 0x7F) {
        if (cap < 1) return DLMS_ERROR_BUFFER;
        dst[0] = (uint8_t)len;
        *w = 1;
    } else if (len <= 0xFF) {
        if (cap < 2) return DLMS_ERROR_BUFFER;
        dst[0] = 0x81;
        dst[1] = (uint8_t)len;
        *w = 2;
    } else if (len <= 0xFFFF) {
        if (cap < 3) return DLMS_ERROR_BUFFER;
        dst[0] = 0x82;
        dst[1] = (uint8_t)(len >> 8);
        dst[2] = (uint8_t)(len & 0xFF);
        *w = 3;
    } else {
        return DLMS_ERROR_INVALID;
    }
    return DLMS_OK;
}

static int decode_length_prefix(const uint8_t *data, size_t len, uint32_t *out, size_t *consumed) {
    if (len < 1) return DLMS_ERROR_BUFFER;
    if (!(data[0] & 0x80)) {
        *out = data[0];
        *consumed = 1;
        return DLMS_OK;
    }
    uint8_t nbytes = (data[0] >> 4) & 0x0F;  /* DLMS: low nibble = byte count */
    if (nbytes == 0 || (size_t)(1 + nbytes) > len) return DLMS_ERROR_PARSE;
    uint32_t val = 0;
    for (uint8_t i = 0; i < nbytes; i++) {
        val = (val << 8) | data[1 + i];
    }
    *out = val;
    *consumed = 1 + nbytes;
    return DLMS_OK;
}

/* ===== Public API ===== */

int dlms_axdr_encode_length(uint32_t length, uint8_t *dst, size_t dst_size, size_t *written) {
    return encode_length_prefix(length, dst, dst_size, written);
}

int dlms_axdr_decode_length(const uint8_t *data, size_t len, uint32_t *length, size_t *consumed) {
    return decode_length_prefix(data, len, length, consumed);
}

dlms_data_type_t dlms_axdr_decode_tag(const uint8_t *data, size_t len, size_t *consumed) {
    if (!data || len < 1) { if (consumed) *consumed = 0; return DLMS_DATA_NULL; }
    if (consumed) *consumed = 1;
    dlms_data_type_t t = (dlms_data_type_t)data[0];
    /* Map known tags */
    switch (t) {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6:
        case 9: case 10: case 12: case 13: case 15: case 16:
        case 17: case 18: case 19: case 20: case 21: case 22:
        case 23: case 24: case 25: case 26: case 27:
        case 255:
            return t;
        default:
            return DLMS_DATA_NULL;
    }
}

int dlms_axdr_encode_tag(dlms_data_type_t type, uint8_t *dst, size_t dst_size, size_t *written) {
    if (!dst || !written) return DLMS_ERROR_INVALID;
    if (dst_size < 1) return DLMS_ERROR_BUFFER;
    dst[0] = (uint8_t)type;
    *written = 1;
    return DLMS_OK;
}

/* ===== Encode ===== */

static int axdr_encode(const dlms_value_t *value, uint8_t *dst, size_t cap, size_t *w) {
    *w = 0;
    size_t pos = 0;

    switch (value->type) {
        case DLMS_DATA_NULL:
        case DLMS_DATA_DONT_CARE:
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = (uint8_t)value->type;
            break;

        case DLMS_DATA_BOOLEAN:
            if (cap < 2) return DLMS_ERROR_BUFFER;
            dst[pos++] = 3;
            dst[pos++] = value->boolean_val ? 1 : 0;
            break;

        case DLMS_DATA_INTEGER:
            if (cap < 2) return DLMS_ERROR_BUFFER;
            dst[pos++] = 15;
            dst[pos++] = (uint8_t)value->int8_val;
            break;

        case DLMS_DATA_UNSIGNED_INTEGER:
            if (cap < 2) return DLMS_ERROR_BUFFER;
            dst[pos++] = 17;
            dst[pos++] = value->uint8_val;
            break;

        case DLMS_DATA_LONG:
            if (cap < 3) return DLMS_ERROR_BUFFER;
            dst[pos++] = 16;
            dst[pos++] = (uint8_t)(value->int16_val >> 8);
            dst[pos++] = (uint8_t)(value->int16_val & 0xFF);
            break;

        case DLMS_DATA_UNSIGNED_LONG:
            if (cap < 3) return DLMS_ERROR_BUFFER;
            dst[pos++] = 18;
            dst[pos++] = (uint8_t)(value->uint16_val >> 8);
            dst[pos++] = (uint8_t)(value->uint16_val & 0xFF);
            break;

        case DLMS_DATA_DOUBLE_LONG:
            if (cap < 5) return DLMS_ERROR_BUFFER;
            dst[pos++] = 5;
            dst[pos++] = (uint8_t)(value->int32_val >> 24);
            dst[pos++] = (uint8_t)(value->int32_val >> 16);
            dst[pos++] = (uint8_t)(value->int32_val >> 8);
            dst[pos++] = (uint8_t)(value->int32_val & 0xFF);
            break;

        case DLMS_DATA_DOUBLE_LONG_UNSIGNED:
            if (cap < 5) return DLMS_ERROR_BUFFER;
            dst[pos++] = 6;
            dst[pos++] = (uint8_t)(value->uint32_val >> 24);
            dst[pos++] = (uint8_t)(value->uint32_val >> 16);
            dst[pos++] = (uint8_t)(value->uint32_val >> 8);
            dst[pos++] = (uint8_t)(value->uint32_val & 0xFF);
            break;

        case DLMS_DATA_LONG64:
            if (cap < 9) return DLMS_ERROR_BUFFER;
            dst[pos++] = 20;
            for (int i = 7; i >= 0; i--)
                dst[pos++] = (uint8_t)(value->int64_val >> (8 * i));
            break;

        case DLMS_DATA_UNSIGNED_LONG64:
            if (cap < 9) return DLMS_ERROR_BUFFER;
            dst[pos++] = 21;
            for (int i = 7; i >= 0; i--)
                dst[pos++] = (uint8_t)(value->uint64_val >> (8 * i));
            break;

        case DLMS_DATA_ENUM:
            if (cap < 2) return DLMS_ERROR_BUFFER;
            dst[pos++] = 22;
            dst[pos++] = value->uint8_val;
            break;

        case DLMS_DATA_FLOAT32:
            if (cap < 5) return DLMS_ERROR_BUFFER;
            dst[pos++] = 23;
            memcpy(dst + pos, &value->float32_val, 4);
            pos += 4;
            break;

        case DLMS_DATA_FLOAT64:
            if (cap < 9) return DLMS_ERROR_BUFFER;
            dst[pos++] = 24;
            memcpy(dst + pos, &value->float64_val, 8);
            pos += 8;
            break;

        case DLMS_DATA_OCTET_STRING: {
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = 9;
            size_t lw = 0;
            int ret = encode_length_prefix(value->octet_string.len, dst + pos, cap - pos, &lw);
            if (ret != DLMS_OK) return ret;
            pos += lw;
            if (pos + value->octet_string.len > cap) return DLMS_ERROR_BUFFER;
            if (value->octet_string.data && value->octet_string.len > 0)
                memcpy(dst + pos, value->octet_string.data, value->octet_string.len);
            pos += value->octet_string.len;
            break;
        }

        case DLMS_DATA_VISIBLE_STRING: {
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = 10;
            size_t lw = 0;
            int ret = encode_length_prefix(value->string_val.len, dst + pos, cap - pos, &lw);
            if (ret != DLMS_OK) return ret;
            pos += lw;
            if (pos + value->string_val.len > cap) return DLMS_ERROR_BUFFER;
            if (value->string_val.data && value->string_val.len > 0)
                memcpy(dst + pos, value->string_val.data, value->string_val.len);
            pos += value->string_val.len;
            break;
        }

        case DLMS_DATA_UTF8_STRING: {
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = 12;
            size_t lw = 0;
            int ret = encode_length_prefix(value->string_val.len, dst + pos, cap - pos, &lw);
            if (ret != DLMS_OK) return ret;
            pos += lw;
            if (pos + value->string_val.len > cap) return DLMS_ERROR_BUFFER;
            if (value->string_val.data && value->string_val.len > 0)
                memcpy(dst + pos, value->string_val.data, value->string_val.len);
            pos += value->string_val.len;
            break;
        }

        case DLMS_DATA_BIT_STRING: {
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = 4;
            size_t lw = 0;
            int ret = encode_length_prefix(value->bit_string.len, dst + pos, cap - pos, &lw);
            if (ret != DLMS_OK) return ret;
            pos += lw;
            if (pos + value->bit_string.len > cap) return DLMS_ERROR_BUFFER;
            if (value->bit_string.data && value->bit_string.len > 0)
                memcpy(dst + pos, value->bit_string.data, value->bit_string.len);
            pos += value->bit_string.len;
            break;
        }

        case DLMS_DATA_DATETIME: {
            if (cap < 13) return DLMS_ERROR_BUFFER;
            dst[pos++] = 25;
            const dlms_value_t *v = value;
            dst[pos++] = (uint8_t)(v->datetime.year >> 8);
            dst[pos++] = (uint8_t)(v->datetime.year & 0xFF);
            dst[pos++] = v->datetime.month;
            dst[pos++] = v->datetime.day;
            dst[pos++] = 0; /* day_of_week: not used in datetime */
            dst[pos++] = v->datetime.hour;
            dst[pos++] = v->datetime.minute;
            dst[pos++] = v->datetime.second;
            dst[pos++] = v->datetime.hundredths;
            dst[pos++] = (uint8_t)(v->datetime.deviation >> 8);
            dst[pos++] = (uint8_t)(v->datetime.deviation & 0xFF);
            dst[pos++] = v->datetime.clock_status;
            break;
        }

        case DLMS_DATA_DATE: {
            if (cap < 6) return DLMS_ERROR_BUFFER;
            dst[pos++] = 26;
            dst[pos++] = (uint8_t)(value->date.year >> 8);
            dst[pos++] = (uint8_t)(value->date.year & 0xFF);
            dst[pos++] = value->date.month;
            dst[pos++] = value->date.day;
            dst[pos++] = value->date.day_of_week;
            break;
        }

        case DLMS_DATA_TIME: {
            if (cap < 5) return DLMS_ERROR_BUFFER;
            dst[pos++] = 27;
            dst[pos++] = value->time_val.hour;
            dst[pos++] = value->time_val.minute;
            dst[pos++] = value->time_val.second;
            dst[pos++] = value->time_val.hundredths;
            break;
        }

        case DLMS_DATA_ARRAY:
        case DLMS_DATA_STRUCTURE: {
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = (uint8_t)value->type;
            size_t lw = 0;
            int ret = encode_length_prefix(value->array_val.count, dst + pos, cap - pos, &lw);
            if (ret != DLMS_OK) return ret;
            pos += lw;
            for (uint16_t i = 0; i < value->array_val.count; i++) {
                size_t iw = 0;
                ret = axdr_encode(&value->array_val.items[i], dst + pos, cap - pos, &iw);
                if (ret != DLMS_OK) return ret;
                pos += iw;
            }
            break;
        }

        case DLMS_DATA_COMPACT_ARRAY: {
            /* Compact array: tag 19, then raw data */
            if (cap < 1) return DLMS_ERROR_BUFFER;
            dst[pos++] = 19;
            /* For now, treat as octet string of the raw content */
            if (value->octet_string.len > 0 && value->octet_string.data) {
                size_t lw = 0;
                int ret = encode_length_prefix(value->octet_string.len, dst + pos, cap - pos, &lw);
                if (ret != DLMS_OK) return ret;
                pos += lw;
                if (pos + value->octet_string.len > cap) return DLMS_ERROR_BUFFER;
                memcpy(dst + pos, value->octet_string.data, value->octet_string.len);
                pos += value->octet_string.len;
            }
            break;
        }

        default:
            return DLMS_ERROR_NOT_SUPPORTED;
    }
    *w = pos;
    return DLMS_OK;
}

int dlms_axdr_encode(const dlms_value_t *value, uint8_t *dst, size_t dst_size, size_t *written) {
    return axdr_encode(value, dst, dst_size, written);
}

/* ===== Decode ===== */

static int axdr_decode(const uint8_t *data, size_t len, dlms_value_t *out, size_t *consumed) {
    if (!data || len < 1 || !out) return DLMS_ERROR_INVALID;
    *consumed = 0;
    size_t pos = 0;
    dlms_data_type_t tag = (dlms_data_type_t)data[pos++];
    out->type = tag;

    switch (tag) {
        case DLMS_DATA_NULL:
        case DLMS_DATA_DONT_CARE:
            memset(out, 0, sizeof(*out));
            out->type = tag;
            *consumed = 1;
            return DLMS_OK;

        case DLMS_DATA_BOOLEAN:
            if (len < 2) return DLMS_ERROR_BUFFER;
            out->boolean_val = data[pos++] ? true : false;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_INTEGER:
            if (len < 2) return DLMS_ERROR_BUFFER;
            out->int8_val = (int8_t)data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_UNSIGNED_INTEGER:
            if (len < 2) return DLMS_ERROR_BUFFER;
            out->uint8_val = data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_ENUM:
            if (len < 2) return DLMS_ERROR_BUFFER;
            out->uint8_val = data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_LONG:
            if (len < 3) return DLMS_ERROR_BUFFER;
            out->int16_val = (int16_t)((uint16_t)data[pos] << 8 | data[pos + 1]);
            pos += 2;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_UNSIGNED_LONG:
            if (len < 3) return DLMS_ERROR_BUFFER;
            out->uint16_val = (uint16_t)data[pos] << 8 | data[pos + 1];
            pos += 2;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_DOUBLE_LONG:
            if (len < 5) return DLMS_ERROR_BUFFER;
            out->int32_val = (int32_t)(((uint32_t)data[pos] << 24) | ((uint32_t)data[pos+1] << 16) |
                                       ((uint32_t)data[pos+2] << 8) | data[pos+3]);
            pos += 4;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_DOUBLE_LONG_UNSIGNED:
            if (len < 5) return DLMS_ERROR_BUFFER;
            out->uint32_val = ((uint32_t)data[pos] << 24) | ((uint32_t)data[pos+1] << 16) |
                              ((uint32_t)data[pos+2] << 8) | data[pos+3];
            pos += 4;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_LONG64:
            if (len < 9) return DLMS_ERROR_BUFFER;
            out->int64_val = 0;
            for (int i = 0; i < 8; i++)
                out->int64_val = (out->int64_val << 8) | data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_UNSIGNED_LONG64:
            if (len < 9) return DLMS_ERROR_BUFFER;
            out->uint64_val = 0;
            for (int i = 0; i < 8; i++)
                out->uint64_val = (out->uint64_val << 8) | data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_FLOAT32:
            if (len < 5) return DLMS_ERROR_BUFFER;
            memcpy(&out->float32_val, data + pos, 4);
            pos += 4;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_FLOAT64:
            if (len < 9) return DLMS_ERROR_BUFFER;
            memcpy(&out->float64_val, data + pos, 8);
            pos += 8;
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_OCTET_STRING:
        case DLMS_DATA_VISIBLE_STRING:
        case DLMS_DATA_UTF8_STRING:
        case DLMS_DATA_BIT_STRING:
        case DLMS_DATA_BCD: {
            uint32_t slen = 0;
            size_t lc = 0;
            int ret = decode_length_prefix(data + pos, len - pos, &slen, &lc);
            if (ret != DLMS_OK) return ret;
            pos += lc;
            if (pos + slen > len) return DLMS_ERROR_BUFFER;
            if (tag == DLMS_DATA_OCTET_STRING || tag == DLMS_DATA_BIT_STRING || tag == DLMS_DATA_BCD) {
                out->octet_string.data = data + pos;
                out->octet_string.len = (uint16_t)slen;
            } else {
                out->string_val.data = (const char *)(data + pos);
                out->string_val.len = (uint16_t)slen;
            }
            pos += slen;
            *consumed = pos;
            return DLMS_OK;
        }

        case DLMS_DATA_DATETIME:
            if (len < 13) return DLMS_ERROR_BUFFER;
            out->datetime.year = (uint16_t)((data[pos] << 8) | data[pos + 1]); pos += 2;
            out->datetime.month = data[pos++];
            out->datetime.day = data[pos++];
            pos++; /* skip day_of_week byte */
            out->datetime.hour = data[pos++];
            out->datetime.minute = data[pos++];
            out->datetime.second = data[pos++];
            out->datetime.hundredths = data[pos++];
            out->datetime.deviation = (int16_t)((data[pos] << 8) | data[pos + 1]); pos += 2;
            out->datetime.clock_status = data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_DATE:
            if (len < 6) return DLMS_ERROR_BUFFER;
            out->date.year = (uint16_t)((data[pos] << 8) | data[pos + 1]); pos += 2;
            out->date.month = data[pos++];
            out->date.day = data[pos++];
            out->date.day_of_week = data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_TIME:
            if (len < 5) return DLMS_ERROR_BUFFER;
            out->time_val.hour = data[pos++];
            out->time_val.minute = data[pos++];
            out->time_val.second = data[pos++];
            out->time_val.hundredths = data[pos++];
            *consumed = pos;
            return DLMS_OK;

        case DLMS_DATA_ARRAY:
        case DLMS_DATA_STRUCTURE: {
            uint32_t count = 0;
            size_t lc = 0;
            int ret = decode_length_prefix(data + pos, len - pos, &count, &lc);
            if (ret != DLMS_OK) return ret;
            pos += lc;
            out->array_val.count = (uint16_t)count;
            out->array_val.items = NULL; /* Caller must manage array storage */
            out->array_val.capacity = (uint16_t)count;
            *consumed = pos;
            return DLMS_OK;
        }

        case DLMS_DATA_COMPACT_ARRAY: {
            uint32_t slen = 0;
            size_t lc = 0;
            int ret = decode_length_prefix(data + pos, len - pos, &slen, &lc);
            if (ret != DLMS_OK) return ret;
            pos += lc;
            if (pos + slen > len) return DLMS_ERROR_BUFFER;
            out->octet_string.data = data + pos;
            out->octet_string.len = (uint16_t)slen;
            pos += slen;
            *consumed = pos;
            return DLMS_OK;
        }

        default:
            return DLMS_ERROR_NOT_SUPPORTED;
    }
}

int dlms_axdr_decode(const uint8_t *data, size_t len, dlms_value_t *out, size_t *consumed) {
    return axdr_decode(data, len, out, consumed);
}

/* ===== Compact Array ===== */

int dlms_axdr_encode_compact_array(const dlms_value_t *items, uint16_t count,
                                   const dlms_data_type_t *types, uint8_t type_count,
                                   uint8_t *dst, size_t dst_size, size_t *written) {
    if (!items || !types || !dst || !written) return DLMS_ERROR_INVALID;
    *written = 0;
    size_t pos = 0;
    if (dst_size < 1) return DLMS_ERROR_BUFFER;

    /* Tag */
    dst[pos++] = 19;

    /* Type description (tag 0) */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 0; /* content description */
    size_t tlw = 0;
    int ret = encode_length_prefix(type_count, dst + pos, dst_size - pos, &tlw);
    if (ret != DLMS_OK) return ret;
    pos += tlw;
    for (uint8_t i = 0; i < type_count; i++) {
        if (pos >= dst_size) return DLMS_ERROR_BUFFER;
        dst[pos++] = (uint8_t)types[i];
    }

    /* Array content (tag 1) */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 1; /* array content */
    size_t clw = 0;
    ret = encode_length_prefix(count, dst + pos, dst_size - pos, &clw);
    if (ret != DLMS_OK) return ret;
    pos += clw;

    /* Encode each item (without tags, just raw values) */
    for (uint16_t i = 0; i < count; i++) {
        size_t iw = 0;
        ret = axdr_encode(&items[i], dst + pos, dst_size - pos, &iw);
        if (ret != DLMS_OK) return ret;
        pos += iw;
    }

    /* Encode overall length */
    *written = pos;
    return DLMS_OK;
}

int dlms_axdr_decode_compact_array(const uint8_t *data, size_t len,
                                   dlms_value_t *items, uint16_t *count, uint16_t max_count,
                                   dlms_data_type_t *types, uint8_t *type_count, uint8_t max_types,
                                   size_t *consumed) {
    if (!data || !items || !count || !types || !type_count || !consumed) return DLMS_ERROR_INVALID;
    *consumed = 0;
    size_t pos = 0;

    if (len < 1 || data[pos++] != 19) return DLMS_ERROR_PARSE;

    /* Skip total length */
    uint32_t total_len = 0;
    size_t lc = 0;
    int ret = decode_length_prefix(data + pos, len - pos, &total_len, &lc);
    if (ret != DLMS_OK) return ret;
    pos += lc;

    /* Content description (tag 0) */
    if (pos >= len || data[pos++] != 0) return DLMS_ERROR_PARSE;
    uint32_t tc = 0;
    ret = decode_length_prefix(data + pos, len - pos, &tc, &lc);
    if (ret != DLMS_OK) return ret;
    pos += lc;
    if (tc > max_types) return DLMS_ERROR_BUFFER;
    *type_count = (uint8_t)tc;
    for (uint8_t i = 0; i < *type_count; i++) {
        if (pos >= len) return DLMS_ERROR_BUFFER;
        types[i] = (dlms_data_type_t)data[pos++];
    }

    /* Array content (tag 1) */
    if (pos >= len || data[pos++] != 1) return DLMS_ERROR_PARSE;
    uint32_t ic = 0;
    ret = decode_length_prefix(data + pos, len - pos, &ic, &lc);
    if (ret != DLMS_OK) return ret;
    pos += lc;
    if (ic > max_count) return DLMS_ERROR_BUFFER;
    *count = (uint16_t)ic;

    for (uint16_t i = 0; i < *count; i++) {
        size_t ic2 = 0;
        ret = axdr_decode(data + pos, len - pos, &items[i], &ic2);
        if (ret != DLMS_OK) return ret;
        pos += ic2;
    }

    *consumed = pos;
    return DLMS_OK;
}

int dlms_axdr_encode_value_list(const dlms_value_t *values, uint16_t count,
                                uint8_t *dst, size_t dst_size, size_t *written) {
    if (!values || !dst || !written) return DLMS_ERROR_INVALID;
    *written = 0;
    size_t pos = 0;
    for (uint16_t i = 0; i < count; i++) {
        size_t iw = 0;
        int ret = axdr_encode(&values[i], dst + pos, dst_size - pos, &iw);
        if (ret != DLMS_OK) return ret;
        pos += iw;
    }
    *written = pos;
    return DLMS_OK;
}
