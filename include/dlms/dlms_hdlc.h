/**
 * @file dlms_hdlc.h
 * @brief HDLC framing for DLMS/COSEM (IEC 62056-46)
 */
#ifndef DLMS_HDLC_H
#define DLMS_HDLC_H

#include "dlms_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DLMS_HDLC_MAX_FRAME_SIZE
#define DLMS_HDLC_MAX_FRAME_SIZE    256
#endif

#ifndef DLMS_HDLC_MAX_INFO_LEN
#define DLMS_HDLC_MAX_INFO_LEN      200
#endif

#define DLMS_HDLC_FLAG              0x7E
#define DLMS_HDLC_ESCAPE            0x7D

/* Frame types */
typedef enum {
    DLMS_HDLC_FRAME_SNRM = 0,
    DLMS_HDLC_FRAME_UA,
    DLMS_HDLC_FRAME_RR,
    DLMS_HDLC_FRAME_INFO,
    DLMS_HDLC_FRAME_DISC,
    DLMS_HDLC_FRAME_UI,
    DLMS_HDLC_FRAME_UNKNOWN
} dlms_hdlc_frame_type_t;

/* HDLC Address */
typedef struct {
    uint16_t logical_address;
    uint16_t physical_address;
    bool has_physical;
    bool extended;
} dlms_hdlc_address_t;

/* HDLC Frame Format (2 bytes) */
typedef struct {
    uint16_t length;     /* 11-bit length */
    bool segmented;      /* segmentation bit */
} dlms_hdlc_format_t;

/* Control field */
typedef struct {
    uint8_t send_seq;    /* 0-7 */
    uint8_t recv_seq;    /* 0-7 */
    bool is_final;
} dlms_hdlc_control_t;

/* Complete HDLC frame */
typedef struct {
    dlms_hdlc_frame_type_t type;
    dlms_hdlc_address_t dest;
    dlms_hdlc_address_t src;
    dlms_hdlc_control_t control;
    const uint8_t *info;
    uint16_t info_len;
    uint16_t hcs;
    uint16_t fcs;
} dlms_hdlc_frame_t;

/* HDLC Parameters for negotiation */
typedef enum {
    DLMS_PARAM_WINDOW_SIZE = 0x01,
    DLMS_PARAM_MAX_INFO_TX = 0x02,
    DLMS_PARAM_MAX_INFO_RX = 0x03,
    DLMS_PARAM_INTER_CHAR_TIMEOUT = 0x04,
    DLMS_PARAM_INACTIVITY_TIMEOUT = 0x05
} dlms_hdlc_param_type_t;

typedef struct {
    dlms_hdlc_param_type_t type;
    uint16_t value;
} dlms_hdlc_param_t;

#define DLMS_HDLC_MAX_PARAMS 5

typedef struct {
    dlms_hdlc_param_t params[DLMS_HDLC_MAX_PARAMS];
    uint8_t count;
} dlms_hdlc_param_list_t;

/* HDLC Parser (streaming, handles fragmentation) */
typedef struct {
    uint8_t *buf;
    size_t buf_cap;
    size_t buf_len;
    enum {
        HDLC_STATE_IDLE,
        HDLC_STATE_RECEIVING,
        HDLC_STATE_ESCAPE
    } state;
} dlms_hdlc_parser_t;

/* Parser callback for received complete frames */
typedef void (*dlms_hdlc_frame_callback_t)(const dlms_hdlc_frame_t *frame, void *user_data);

/* CRC-16 CCITT */
uint16_t dlms_crc16(const uint8_t *data, size_t len);
uint16_t dlms_crc16_update(uint16_t crc, const uint8_t *data, size_t len);

/* HDLC Address */
int dlms_hdlc_address_encode(const dlms_hdlc_address_t *addr, bool is_client,
                             uint8_t *dst, size_t dst_size, size_t *written);
int dlms_hdlc_address_decode(const uint8_t *data, size_t len, bool is_client,
                             dlms_hdlc_address_t *addr, size_t *consumed);

/* Frame Format */
int dlms_hdlc_format_encode(const dlms_hdlc_format_t *fmt, uint8_t *dst);
int dlms_hdlc_format_decode(const uint8_t *data, dlms_hdlc_format_t *fmt);

/* Control Field */
int dlms_hdlc_control_encode_snrm(uint8_t *dst);
int dlms_hdlc_control_encode_ua(uint8_t *dst);
int dlms_hdlc_control_encode_rr(uint8_t recv_seq, uint8_t *dst);
int dlms_hdlc_control_encode_info(uint8_t send_seq, uint8_t recv_seq, bool final, uint8_t *dst);
int dlms_hdlc_control_encode_disc(uint8_t *dst);
int dlms_hdlc_control_encode_ui(bool final, uint8_t *dst);
int dlms_hdlc_control_decode(uint8_t byte, dlms_hdlc_frame_type_t type,
                             dlms_hdlc_control_t *control);

/* Frame encode/decode */
int dlms_hdlc_frame_encode(uint8_t *dst, size_t dst_size, const dlms_hdlc_frame_t *frame);
int dlms_hdlc_frame_decode(const uint8_t *data, size_t len, dlms_hdlc_frame_t *frame);

/* HDLC byte stuffing/unstuffing */
size_t dlms_hdlc_stuff(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap);
size_t dlms_hdlc_unstuff(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap);

/* Streaming parser */
dlms_hdlc_parser_t *dlms_hdlc_parser_create(uint8_t *buf, size_t buf_size);
void dlms_hdlc_parser_reset(dlms_hdlc_parser_t *parser);
int dlms_hdlc_parse(dlms_hdlc_parser_t *parser, const uint8_t *data, size_t len,
                    dlms_hdlc_frame_callback_t callback, void *user_data);

/* HDLC Parameters */
int dlms_hdlc_param_list_init(dlms_hdlc_param_list_t *list);
int dlms_hdlc_param_list_set(dlms_hdlc_param_list_t *list, dlms_hdlc_param_type_t type, uint16_t value);
int dlms_hdlc_param_list_get(const dlms_hdlc_param_list_t *list, dlms_hdlc_param_type_t type, uint16_t *value);
int dlms_hdlc_param_list_encode(const dlms_hdlc_param_list_t *list, uint8_t *dst, size_t dst_size, size_t *written);
int dlms_hdlc_param_list_decode(const uint8_t *data, size_t len, dlms_hdlc_param_list_t *list);

/* Frame builder helpers */
int dlms_hdlc_build_snrm(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                         const dlms_hdlc_address_t *src, const dlms_hdlc_param_list_t *params);
int dlms_hdlc_build_ua(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                       const dlms_hdlc_address_t *src, const dlms_hdlc_param_list_t *params);
int dlms_hdlc_build_rr(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                       const dlms_hdlc_address_t *src, uint8_t recv_seq);
int dlms_hdlc_build_info(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                         const dlms_hdlc_address_t *src, uint8_t send_seq, uint8_t recv_seq,
                         bool final, const uint8_t *info, uint16_t info_len);
int dlms_hdlc_build_disc(uint8_t *dst, size_t dst_size, const dlms_hdlc_address_t *dest,
                         const dlms_hdlc_address_t *src);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_HDLC_H */
