/**
 * @file test_main.c
 * @brief Test framework and runner
 */
#include <stdio.h>
#include "test_macros.h"

int current_suite_failed = 0;

#define TEST(name) do { \
    tests_run++; \
    current_suite_failed = 0; \
    name(); \
    if (current_suite_failed) tests_failed++; else tests_passed++; \
} while(0)

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

extern void test_core_obis(void);
extern void test_core_buffer(void);
extern void test_core_varint(void);
extern void test_core_value(void);
extern void test_core_data_type(void);
extern void test_core_bits(void);
extern void test_hdlc_crc(void);
extern void test_hdlc_stuff(void);
extern void test_hdlc_address(void);
extern void test_hdlc_format(void);
extern void test_hdlc_control(void);
extern void test_hdlc_params(void);
extern void test_hdlc_frame(void);
extern void test_hdlc_parser(void);
extern void test_hdlc_build(void);
extern void test_axdr_encode(void);
extern void test_axdr_decode(void);
extern void test_axdr_length(void);
extern void test_axdr_compact(void);
extern void test_asn1_ber(void);
extern void test_asn1_aarq(void);
extern void test_asn1_aare(void);
extern void test_security_sm4(void);
extern void test_security_sm4_modes(void);
extern void test_security_aes(void);
extern void test_security_gcm(void);
extern void test_security_control(void);
extern void test_security_dlms(void);
extern void test_security_hls(void);
extern void test_cosem_data(void);
extern void test_cosem_register(void);
extern void test_cosem_clock(void);
extern void test_cosem_security_setup(void);
extern void test_cosem_profile_generic(void);
extern void test_cosem_list(void);
extern void test_cosem_association(void);

int main(void) {
    printf("=== DLMS/COSEM C99 Library Tests ===\n\n");

    printf("--- Core Tests ---\n");
    TEST(test_core_obis);
    TEST(test_core_buffer);
    TEST(test_core_varint);
    TEST(test_core_value);
    TEST(test_core_data_type);
    TEST(test_core_bits);

    printf("--- HDLC Tests ---\n");
    TEST(test_hdlc_crc);
    TEST(test_hdlc_stuff);
    TEST(test_hdlc_address);
    TEST(test_hdlc_format);
    TEST(test_hdlc_control);
    TEST(test_hdlc_params);
    TEST(test_hdlc_frame);
    TEST(test_hdlc_parser);
    TEST(test_hdlc_build);

    printf("--- AXDR Tests ---\n");
    TEST(test_axdr_encode);
    TEST(test_axdr_decode);
    TEST(test_axdr_length);
    TEST(test_axdr_compact);

    printf("--- ASN1 Tests ---\n");
    TEST(test_asn1_ber);
    TEST(test_asn1_aarq);
    TEST(test_asn1_aare);

    printf("--- Security Tests ---\n");
    TEST(test_security_sm4);
    TEST(test_security_sm4_modes);
    TEST(test_security_aes);
    TEST(test_security_gcm);
    TEST(test_security_control);
    TEST(test_security_dlms);
    TEST(test_security_hls);

    printf("--- COSEM Tests ---\n");
    TEST(test_cosem_data);
    TEST(test_cosem_register);
    TEST(test_cosem_clock);
    TEST(test_cosem_security_setup);
    TEST(test_cosem_profile_generic);
    TEST(test_cosem_list);
    TEST(test_cosem_association);

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
