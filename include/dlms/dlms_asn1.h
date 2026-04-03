/**
 * @file dlms_asn1.h
 * @brief BER-TLV encoding for AARQ/AARE (DLMS application layer)
 */
#ifndef DLMS_ASN1_H
#define DLMS_ASN1_H

#include "dlms_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BER Tag classes */
typedef enum {
    DLMS_BER_CLASS_UNIVERSAL = 0x00,
    DLMS_BER_CLASS_APPLICATION = 0x40,
    DLMS_BER_CLASS_CONTEXT = 0x80,
    DLMS_BER_CLASS_PRIVATE = 0xC0
} dlms_ber_class_t;

/* BER Universal Tags */
typedef enum {
    DLMS_BER_TAG_BOOLEAN = 0x01,
    DLMS_BER_TAG_INTEGER = 0x02,
    DLMS_BER_TAG_BIT_STRING = 0x03,
    DLMS_BER_TAG_OCTET_STRING = 0x04,
    DLMS_BER_TAG_NULL = 0x05,
    DLMS_BER_TAG_OID = 0x06,
    DLMS_BER_TAG_UTF8_STRING = 0x0C,
    DLMS_BER_TAG_SEQUENCE = 0x30,
    DLMS_BER_TAG_SET = 0x31
} dlms_ber_universal_tag_t;

/* Application-level tags for DLMS */
#define DLMS_ASN1_TAG_AARQ    0x60
#define DLMS_ASN1_TAG_AARE    0x61
#define DLMS_ASN1_TAG_RLRQ    0x62
#define DLMS_ASN1_TAG_RLRE    0x63
#define DLMS_ASN1_TAG_GET_REQ 0xC0
#define DLMS_ASN1_TAG_GET_RES 0xC1
#define DLMS_ASN1_TAG_SET_REQ 0xC2
#define DLMS_ASN1_TAG_SET_RES 0xC3
#define DLMS_ASN1_TAG_ACT_REQ 0xC4
#define DLMS_ASN1_TAG_ACT_RES 0xC5

/* BER-TLV header */
typedef struct {
    uint8_t tag;
    uint32_t length;
    bool constructed;
} dlms_ber_header_t;

/* BER encoding/decoding */
int dlms_ber_encode_tag(uint8_t tag, uint8_t *dst, size_t dst_size, size_t *written);
int dlms_ber_decode_tag(const uint8_t *data, size_t len, dlms_ber_header_t *header, size_t *consumed);
int dlms_ber_encode_length(uint32_t length, uint8_t *dst, size_t dst_size, size_t *written);
int dlms_ber_decode_length(const uint8_t *data, size_t len, uint32_t *length, size_t *consumed);

/* AARQ - Association Request */
typedef struct {
    uint8_t dlms_version;          /* typically 6 */
    dlms_auth_mechanism_t auth_mech;
    const uint8_t *auth_value;     /* password/challenge */
    uint16_t auth_value_len;
    const uint8_t *system_title;
    uint8_t system_title_len;
    uint16_t proposed_conformance;
    uint16_t proposed_max_pdu_size;
    const char *client_ap_title;
    uint16_t client_ap_title_len;
} dlms_aarq_t;

int dlms_aarq_encode(const dlms_aarq_t *aarq, uint8_t *dst, size_t dst_size, size_t *written);
int dlms_aarq_decode(const uint8_t *data, size_t len, dlms_aarq_t *aarq, size_t *consumed);

/* AARE - Association Response */
typedef struct {
    uint8_t result;                /* 0=accepted, 1=rejected permanent, 2=rejected transient */
    uint8_t result_source_diagnostics;
    uint8_t dlms_version;
    dlms_auth_mechanism_t auth_mech;
    const uint8_t *auth_value;
    uint16_t auth_value_len;
    const uint8_t *system_title;
    uint8_t system_title_len;
    uint16_t negotiated_conformance;
    uint16_t negotiated_max_pdu_size;
    uint8_t server_ap_title[64];
    uint8_t server_ap_title_len;
} dlms_aare_t;

int dlms_aare_decode(const uint8_t *data, size_t len, dlms_aare_t *aare, size_t *consumed);

/* Initiate Request/Response (A-XDR encoded, used within AARQ/AARE) */
typedef struct {
    uint8_t dlms_version;
    dlms_auth_mechanism_t auth_mech;
    const uint8_t *auth_value;
    uint16_t auth_value_len;
    const uint8_t *c_to_s_challenge;
    uint16_t c_to_s_challenge_len;
    const uint8_t *s_to_c_challenge;
    uint16_t s_to_c_challenge_len;
    uint16_t proposed_conformance;
    uint16_t proposed_max_pdu_size;
} dlms_initiate_request_t;

typedef struct {
    uint8_t dlms_version;
    dlms_auth_mechanism_t auth_mech;
    const uint8_t *auth_value;
    uint16_t auth_value_len;
    uint8_t server_system_title[8];
    uint8_t server_system_title_len;
    uint16_t negotiated_conformance;
    uint16_t negotiated_max_pdu_size;
    uint16_t negotiated_window_size;
    const uint8_t *s_to_c_challenge;
    uint16_t s_to_c_challenge_len;
} dlms_initiate_response_t;

int dlms_initiate_request_encode(const dlms_initiate_request_t *req,
                                 uint8_t *dst, size_t dst_size, size_t *written);
int dlms_initiate_response_decode(const uint8_t *data, size_t len,
                                  dlms_initiate_response_t *resp, size_t *consumed);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_ASN1_H */
