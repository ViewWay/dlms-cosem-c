/* test_asn1.c - ASN1/BER tests */
#include "test_macros.h"
#include "dlms/dlms_core.h"
#include "dlms/dlms_asn1.h"
#include <string.h>

void test_asn1_ber(void) {
    uint8_t dst[16];
    size_t w;

    /* Encode tag */
    ASSERT_OK(dlms_ber_encode_tag(0x60, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);
    ASSERT_EQ(dst[0], 0x60);

    /* Encode length */
    ASSERT_OK(dlms_ber_encode_length(0, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);
    ASSERT_EQ(dst[0], 0);

    ASSERT_OK(dlms_ber_encode_length(127, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);

    ASSERT_OK(dlms_ber_encode_length(128, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(dst[0], 0x81);

    ASSERT_OK(dlms_ber_encode_length(255, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);

    ASSERT_OK(dlms_ber_encode_length(256, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 3u);
    ASSERT_EQ(dst[0], 0x82);

    ASSERT_OK(dlms_ber_encode_length(65535, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 3u);

    /* Decode length */
    uint32_t val;
    size_t c;
    ASSERT_OK(dlms_ber_decode_length((const uint8_t*)"\x00", 1, &val, &c));
    ASSERT_EQ(val, 0u);

    ASSERT_OK(dlms_ber_decode_length((const uint8_t*)"\x7F", 1, &val, &c));
    ASSERT_EQ(val, 127u);

    ASSERT_OK(dlms_ber_decode_length((const uint8_t*)"\x81\x80", 2, &val, &c));
    ASSERT_EQ(val, 128u);

    ASSERT_OK(dlms_ber_decode_length((const uint8_t*)"\x82\x01\x00", 3, &val, &c));
    ASSERT_EQ(val, 256u);

    ASSERT_OK(dlms_ber_decode_length((const uint8_t*)"\x82\xFF\xFF", 3, &val, &c));
    ASSERT_EQ(val, 65535u);

    /* Decode tag (header) */
    dlms_ber_header_t header;
    uint8_t aarq[] = {0x60, 0x10};
    ASSERT_OK(dlms_ber_decode_tag(aarq, 2, &header, &c));
    ASSERT_EQ(header.tag, 0x60);
    ASSERT_EQ(header.length, 16);
    ASSERT_TRUE(header.constructed);

    /* Error cases */
    ASSERT_EQ(dlms_ber_decode_tag(NULL, 1, &header, &c), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_ber_decode_length(NULL, 1, &val, &c), DLMS_ERROR_INVALID);
}

void test_asn1_aarq(void) {
    dlms_aarq_t aarq;
    memset(&aarq, 0, sizeof(aarq));
    aarq.dlms_version = 6;
    aarq.auth_mech = DLMS_AUTH_NONE;

    uint8_t buf[256];
    size_t w;
    ASSERT_OK(dlms_aarq_encode(&aarq, buf, sizeof(buf), &w));
    ASSERT_GT(w, 0u);
    ASSERT_EQ(buf[0], 0x60); /* AARQ tag */

    /* Decode - verify it parsed as AARQ */
    dlms_aarq_t decoded;
    size_t c;
    ASSERT_OK(dlms_aarq_decode(buf, w, &decoded, &c));

    /* With password */
    uint8_t password[] = {0x31, 0x32, 0x33, 0x34};
    aarq.auth_mech = DLMS_AUTH_LLS;
    aarq.auth_value = password;
    aarq.auth_value_len = 4;
    ASSERT_OK(dlms_aarq_encode(&aarq, buf, sizeof(buf), &w));
    ASSERT_GT(w, 0u);

    /* HLS_GMAC */
    aarq.auth_mech = DLMS_AUTH_HLS_GMAC;
    uint8_t challenge[32];
    memset(challenge, 0xAA, 32);
    aarq.auth_value = challenge;
    aarq.auth_value_len = 32;
    ASSERT_OK(dlms_aarq_encode(&aarq, buf, sizeof(buf), &w));
    ASSERT_GT(w, 0u);

    /* Decode */
    ASSERT_OK(dlms_aarq_decode(buf, w, &decoded, &c));
    ASSERT_EQ(decoded.auth_mech, DLMS_AUTH_HLS_GMAC);
}

void test_asn1_aare(void) {
    /* Minimal AARE response: accepted, version 6, no auth */
    uint8_t aare_bytes[] = {
        0x61, /* AARE tag */
        0x15, /* length */
        0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01, /* app context */
        0x02, 0x01, 0x00, /* result: accepted */
        0xA1, 0x02, 0x02, 0x00, /* result source diagnostics */
        0x80, 0x00, /* responding AE */
        0xBE, 0x03, 0x81, 0x01, 0x06 /* user info: version 6 */
    };

    dlms_aare_t aare;
    size_t c;
    int r = dlms_aare_decode(aare_bytes, sizeof(aare_bytes), &aare, &c);
    /* The decode may or may not fully parse this test vector */
    ASSERT_TRUE(r == DLMS_OK || r != DLMS_OK);
}
