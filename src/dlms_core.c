/**
 * @file dlms_core.c
 * @brief DLMS/COSEM core types and utilities implementation
 */
#include "dlms/dlms_core.h"
#include <stdio.h>

/* ===== OBIS ===== */

int dlms_obis_parse(const uint8_t *buf, size_t len, dlms_obis_t *obis) {
    if (!buf || !obis || len < 6) return DLMS_ERROR_INVALID;
    memcpy(obis->bytes, buf, 6);
    return DLMS_OK;
}

int dlms_obis_to_string(const dlms_obis_t *obis, char *buf, size_t buf_size) {
    if (!obis || !buf || buf_size < 20) return DLMS_ERROR_BUFFER;
    int n = snprintf(buf, buf_size, "%d.%d.%d.%d.%d.%d",
                     obis->bytes[0], obis->bytes[1], obis->bytes[2],
                     obis->bytes[3], obis->bytes[4], obis->bytes[5]);
    return (n > 0 && (size_t)n < buf_size) ? DLMS_OK : DLMS_ERROR_BUFFER;
}

int dlms_obis_from_string(const char *str, dlms_obis_t *obis) {
    if (!str || !obis) return DLMS_ERROR_INVALID;
    unsigned int b[6];
    int n = sscanf(str, "%u.%u.%u.%u.%u.%u", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    if (n != 6) return DLMS_ERROR_PARSE;
    for (int i = 0; i < 6; i++) {
        if (b[i] > 255) return DLMS_ERROR_INVALID;
        obis->bytes[i] = (uint8_t)b[i];
    }
    return DLMS_OK;
}

bool dlms_obis_equals(const dlms_obis_t *a, const dlms_obis_t *b) {
    if (!a || !b) return false;
    return memcmp(a->bytes, b->bytes, 6) == 0;
}

bool dlms_obis_is_wildcard(const dlms_obis_t *obis) {
    if (!obis) return false;
    for (int i = 0; i < 6; i++) {
        if (obis->bytes[i] != 0xFF) return false;
    }
    return true;
}

/* ===== Buffer ===== */

void dlms_buffer_init(dlms_buffer_t *buf, uint8_t *data, size_t cap) {
    if (!buf) return;
    buf->data = data;
    buf->len = 0;
    buf->cap = cap;
}

int dlms_buffer_append(dlms_buffer_t *buf, const uint8_t *data, size_t len) {
    if (!buf || !data) return DLMS_ERROR_INVALID;
    if (buf->len + len > buf->cap) return DLMS_ERROR_BUFFER;
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    return DLMS_OK;
}

int dlms_buffer_append_byte(dlms_buffer_t *buf, uint8_t byte) {
    return dlms_buffer_append(buf, &byte, 1);
}

void dlms_buffer_clear(dlms_buffer_t *buf) {
    if (buf) buf->len = 0;
}

int dlms_buffer_remaining(const dlms_buffer_t *buf) {
    if (!buf) return 0;
    return (int)(buf->cap - buf->len);
}

/* ===== Variable-length integer ===== */

int dlms_varint_encode(uint32_t value, uint8_t *dst, size_t dst_size, size_t *written) {
    if (!dst || !written) return DLMS_ERROR_INVALID;
    if (value <= 0x7F) {
        if (dst_size < 1) return DLMS_ERROR_BUFFER;
        dst[0] = (uint8_t)value;
        *written = 1;
        return DLMS_OK;
    }
    /* Determine number of bytes needed for value */
    uint8_t nbytes = 0;
    uint32_t tmp = value;
    while (tmp > 0) { nbytes++; tmp >>= 8; }
    if (dst_size < (size_t)(1 + nbytes)) return DLMS_ERROR_BUFFER;
    dst[0] = 0x80 | nbytes;
    for (uint8_t i = 0; i < nbytes; i++) {
        dst[1 + i] = (uint8_t)(value >> (8 * (nbytes - 1 - i)));
    }
    *written = 1 + nbytes;
    return DLMS_OK;
}

int dlms_varint_decode(const uint8_t *data, size_t len, uint32_t *value, size_t *consumed) {
    if (!data || !value || !consumed || len < 1) return DLMS_ERROR_INVALID;
    uint8_t first = data[0];
    if (!(first & 0x80)) {
        *value = first;
        *consumed = 1;
        return DLMS_OK;
    }
    uint8_t nbytes = first & 0x7F;
    if (nbytes == 0 || (size_t)(1 + nbytes) > len) return DLMS_ERROR_PARSE;
    if (nbytes > 4) return DLMS_ERROR_PARSE;
    uint32_t val = 0;
    for (uint8_t i = 0; i < nbytes; i++) {
        val = (val << 8) | data[1 + i];
    }
    *value = val;
    *consumed = 1 + nbytes;
    return DLMS_OK;
}

/* ===== Data type utilities ===== */

const char *dlms_data_type_name(dlms_data_type_t type) {
    switch (type) {
        case DLMS_DATA_NULL: return "null";
        case DLMS_DATA_ARRAY: return "array";
        case DLMS_DATA_STRUCTURE: return "structure";
        case DLMS_DATA_BOOLEAN: return "boolean";
        case DLMS_DATA_BIT_STRING: return "bit_string";
        case DLMS_DATA_DOUBLE_LONG: return "double_long";
        case DLMS_DATA_DOUBLE_LONG_UNSIGNED: return "double_long_unsigned";
        case DLMS_DATA_OCTET_STRING: return "octet_string";
        case DLMS_DATA_VISIBLE_STRING: return "visible_string";
        case DLMS_DATA_UTF8_STRING: return "utf8_string";
        case DLMS_DATA_BCD: return "bcd";
        case DLMS_DATA_INTEGER: return "integer";
        case DLMS_DATA_LONG: return "long";
        case DLMS_DATA_UNSIGNED_INTEGER: return "unsigned_integer";
        case DLMS_DATA_UNSIGNED_LONG: return "unsigned_long";
        case DLMS_DATA_COMPACT_ARRAY: return "compact_array";
        case DLMS_DATA_LONG64: return "long64";
        case DLMS_DATA_UNSIGNED_LONG64: return "unsigned_long64";
        case DLMS_DATA_ENUM: return "enum";
        case DLMS_DATA_FLOAT32: return "float32";
        case DLMS_DATA_FLOAT64: return "float64";
        case DLMS_DATA_DATETIME: return "datetime";
        case DLMS_DATA_DATE: return "date";
        case DLMS_DATA_TIME: return "time";
        case DLMS_DATA_DONT_CARE: return "dont_care";
        default: return "unknown";
    }
}

uint16_t dlms_data_type_fixed_length(dlms_data_type_t type) {
    switch (type) {
        case DLMS_DATA_NULL: return 0;
        case DLMS_DATA_BOOLEAN: return 1;
        case DLMS_DATA_DOUBLE_LONG: return 4;
        case DLMS_DATA_DOUBLE_LONG_UNSIGNED: return 4;
        case DLMS_DATA_INTEGER: return 1;
        case DLMS_DATA_LONG: return 2;
        case DLMS_DATA_UNSIGNED_INTEGER: return 1;
        case DLMS_DATA_UNSIGNED_LONG: return 2;
        case DLMS_DATA_LONG64: return 8;
        case DLMS_DATA_UNSIGNED_LONG64: return 8;
        case DLMS_DATA_ENUM: return 1;
        case DLMS_DATA_FLOAT32: return 4;
        case DLMS_DATA_FLOAT64: return 8;
        case DLMS_DATA_DATETIME: return 12;
        case DLMS_DATA_DATE: return 5;
        case DLMS_DATA_TIME: return 4;
        case DLMS_DATA_DONT_CARE: return 0;
        default: return 0xFFFF; /* variable length */
    }
}

int dlms_reverse_bits8(uint8_t b) {
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
}

uint16_t dlms_reverse_bits16(uint16_t v) {
    return ((uint16_t)dlms_reverse_bits8((uint8_t)(v >> 8))) |
           ((uint16_t)dlms_reverse_bits8((uint8_t)v) << 8);
}

/* ===== Value setters ===== */

void dlms_value_init(dlms_value_t *v) {
    if (!v) return;
    memset(v, 0, sizeof(*v));
    v->type = DLMS_DATA_NULL;
}

void dlms_value_set_null(dlms_value_t *v) { dlms_value_init(v); v->type = DLMS_DATA_NULL; }
void dlms_value_set_bool(dlms_value_t *v, bool val) { dlms_value_init(v); v->type = DLMS_DATA_BOOLEAN; v->boolean_val = val; }
void dlms_value_set_int8(dlms_value_t *v, int8_t val) { dlms_value_init(v); v->type = DLMS_DATA_INTEGER; v->int8_val = val; }
void dlms_value_set_uint8(dlms_value_t *v, uint8_t val) { dlms_value_init(v); v->type = DLMS_DATA_UNSIGNED_INTEGER; v->uint8_val = val; }
void dlms_value_set_int16(dlms_value_t *v, int16_t val) { dlms_value_init(v); v->type = DLMS_DATA_LONG; v->int16_val = val; }
void dlms_value_set_uint16(dlms_value_t *v, uint16_t val) { dlms_value_init(v); v->type = DLMS_DATA_UNSIGNED_LONG; v->uint16_val = val; }
void dlms_value_set_int32(dlms_value_t *v, int32_t val) { dlms_value_init(v); v->type = DLMS_DATA_DOUBLE_LONG; v->int32_val = val; }
void dlms_value_set_uint32(dlms_value_t *v, uint32_t val) { dlms_value_init(v); v->type = DLMS_DATA_DOUBLE_LONG_UNSIGNED; v->uint32_val = val; }
void dlms_value_set_int64(dlms_value_t *v, int64_t val) { dlms_value_init(v); v->type = DLMS_DATA_LONG64; v->int64_val = val; }
void dlms_value_set_uint64(dlms_value_t *v, uint64_t val) { dlms_value_init(v); v->type = DLMS_DATA_UNSIGNED_LONG64; v->uint64_val = val; }
void dlms_value_set_enum(dlms_value_t *v, uint8_t val) { dlms_value_init(v); v->type = DLMS_DATA_ENUM; v->uint8_val = val; }
void dlms_value_set_octet(dlms_value_t *v, const uint8_t *data, uint16_t len) {
    dlms_value_init(v); v->type = DLMS_DATA_OCTET_STRING; v->octet_string.data = data; v->octet_string.len = len;
}
void dlms_value_set_string(dlms_value_t *v, const char *data, uint16_t len) {
    dlms_value_init(v); v->type = DLMS_DATA_VISIBLE_STRING; v->string_val.data = data; v->string_val.len = len;
}
void dlms_value_set_float32(dlms_value_t *v, float val) { dlms_value_init(v); v->type = DLMS_DATA_FLOAT32; v->float32_val = val; }
void dlms_value_set_float64(dlms_value_t *v, double val) { dlms_value_init(v); v->type = DLMS_DATA_FLOAT64; v->float64_val = val; }
