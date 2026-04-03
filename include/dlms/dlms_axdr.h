/**
 * @file dlms_axdr.h
 * @brief A-XDR (Adapted External Data Representation) encoding/decoding
 */
#ifndef DLMS_AXDR_H
#define DLMS_AXDR_H

#include "dlms_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Encode a DLMS value to A-XDR format */
int dlms_axdr_encode(const dlms_value_t *value, uint8_t *dst, size_t dst_size, size_t *written);

/* Decode a DLMS value from A-XDR format */
int dlms_axdr_decode(const uint8_t *data, size_t len, dlms_value_t *out, size_t *consumed);

/* Decode data length prefix (variable-length integer) */
int dlms_axdr_decode_length(const uint8_t *data, size_t len, uint32_t *length, size_t *consumed);

/* Encode data length prefix */
int dlms_axdr_encode_length(uint32_t length, uint8_t *dst, size_t dst_size, size_t *written);

/* Decode tag byte and return data type */
dlms_data_type_t dlms_axdr_decode_tag(const uint8_t *data, size_t len, size_t *consumed);

/* Encode tag byte */
int dlms_axdr_encode_tag(dlms_data_type_t type, uint8_t *dst, size_t dst_size, size_t *written);

/* Compact Array support */
typedef struct {
    dlms_data_type_t *types;
    uint8_t type_count;
    uint8_t raw_data[1]; /* flexible array member placeholder */
} dlms_compact_array_desc_t;

int dlms_axdr_encode_compact_array(const dlms_value_t *items, uint16_t count,
                                   const dlms_data_type_t *types, uint8_t type_count,
                                   uint8_t *dst, size_t dst_size, size_t *written);
int dlms_axdr_decode_compact_array(const uint8_t *data, size_t len,
                                   dlms_value_t *items, uint16_t *count, uint16_t max_count,
                                   dlms_data_type_t *types, uint8_t *type_count, uint8_t max_types,
                                   size_t *consumed);

/* High-level convenience: encode/decode octet string of values */
int dlms_axdr_encode_value_list(const dlms_value_t *values, uint16_t count,
                                uint8_t *dst, size_t dst_size, size_t *written);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_AXDR_H */
