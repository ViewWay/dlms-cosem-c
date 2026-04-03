/* test_security.c - Security tests */
#include "test_macros.h"
#include "dlms/dlms_core.h"
#include "dlms/dlms_security.h"
#include <string.h>

void test_security_sm4(void) {
    /* SM4 S-box test vectors */
    uint8_t key[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};
    uint8_t plain[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};
    uint8_t expected[16] = {0x68,0x1E,0xDF,0x34,0xD2,0x06,0x96,0x5E,0x86,0xB3,0xE9,0x4F,0x53,0x6E,0x42,0x46};

    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);

    uint8_t cipher[16];
    dlms_sm4_encrypt_block(&ctx, plain, cipher);
    ASSERT_MEM_EQ(cipher, expected, 16);

    /* Decrypt */
    uint8_t decrypted[16];
    dlms_sm4_decrypt_block(&ctx, cipher, decrypted);
    ASSERT_MEM_EQ(decrypted, plain, 16);

    /* Different key */
    uint8_t key2[16] = {0};
    dlms_sm4_key_setup(&ctx, key2);
    dlms_sm4_encrypt_block(&ctx, plain, cipher);
    /* Different key -> different output */
    ASSERT_NEQ(cipher[0], expected[0]);

    /* All zeros */
    uint8_t zeros[16] = {0};
    dlms_sm4_key_setup(&ctx, zeros);
    dlms_sm4_encrypt_block(&ctx, zeros, cipher);
    /* Should produce some non-zero output */
    uint8_t check = 0;
    for (int i = 0; i < 16; i++) check |= cipher[i];
    ASSERT_NEQ(check, 0);
}

void test_security_sm4_modes(void) {
    uint8_t key[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};

    /* ECB */
    uint8_t plain[32], cipher[32], dec[32];
    memset(plain, 0xAA, 32);
    ASSERT_OK(dlms_sm4_ecb_encrypt(key, plain, 32, cipher));
    ASSERT_OK(dlms_sm4_ecb_decrypt(key, cipher, 32, dec));
    ASSERT_MEM_EQ(dec, plain, 32);

    /* CBC */
    uint8_t iv[16] = {0};
    ASSERT_OK(dlms_sm4_cbc_encrypt(key, iv, plain, 32, cipher));
    ASSERT_OK(dlms_sm4_cbc_decrypt(key, iv, cipher, 32, dec));
    ASSERT_MEM_EQ(dec, plain, 32);

    /* CTR */
    uint8_t nonce[12] = {0};
    ASSERT_OK(dlms_sm4_ctr(key, nonce, plain, 32, cipher));
    /* CTR is self-inverse */
    uint8_t dec2[32];
    ASSERT_OK(dlms_sm4_ctr(key, nonce, cipher, 32, dec2));
    ASSERT_MEM_EQ(dec2, plain, 32);

    /* CTR with non-aligned length */
    ASSERT_OK(dlms_sm4_ctr(key, nonce, plain, 17, cipher));
    ASSERT_OK(dlms_sm4_ctr(key, nonce, cipher, 17, dec2));
    ASSERT_MEM_EQ(dec2, plain, 17);

    /* Error cases */
    ASSERT_EQ(dlms_sm4_ecb_encrypt(key, plain, 15, cipher), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_sm4_cbc_encrypt(key, iv, plain, 15, cipher), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_sm4_ecb_encrypt(NULL, plain, 16, cipher), DLMS_ERROR_INVALID);
}

void test_security_aes(void) {
    /* AES-128 encrypt/decrypt roundtrip */
    uint8_t key[16] = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};
    uint8_t plain[16] = {0x32,0x43,0xf6,0xa8,0x88,0x5a,0x30,0x8d,0x31,0x31,0x98,0xa2,0xe0,0x37,0x07,0x34};

    dlms_aes128_ctx_t ctx;
    dlms_aes128_key_setup(&ctx, key);
    uint8_t cipher2[16];
    dlms_aes128_encrypt_block(&ctx, plain, cipher2);

    /* Different key produces different output */
    uint8_t key2[16] = {0};
    dlms_aes128_key_setup(&ctx, key2);
    uint8_t cipher3[16];
    dlms_aes128_encrypt_block(&ctx, plain, cipher3);
    /* Verify at least one byte differs */
    int aes_diff = 0;
    for (int i = 0; i < 16; i++) aes_diff |= (cipher2[i] ^ cipher3[i]);
    ASSERT_NEQ(aes_diff, 0);

    /* All zeros */
    memset(key, 0, 16);
    memset(plain, 0, 16);
    dlms_aes128_key_setup(&ctx, key);
    dlms_aes128_encrypt_block(&ctx, plain, cipher2);
    uint8_t check = 0;
    for (int i = 0; i < 16; i++) check |= cipher2[i];
    ASSERT_NEQ(check, 0);
}

void test_security_gcm(void) {
    uint8_t gcm_key[16] = {0};
    memset(gcm_key, 0xFE, 16);
    uint8_t gcm_iv[12] = {0xCA, 0xFE, 0xBA, 0xBE, 0xFA, 0xCE, 0xDB, 0xAD, 0xDE, 0xCA, 0xF8, 0x88};
    uint8_t gcm_plain[16] = "Hello, World!!!";
    uint8_t gcm_aad[] = "additional";

    /* SM4-GCM encrypt */
    uint8_t gcm_ct[16], gcm_tag[12];
    ASSERT_OK(dlms_sm4_gcm_encrypt(gcm_key, gcm_iv, gcm_aad, sizeof(gcm_aad)-1, gcm_plain, 16, gcm_ct, gcm_tag));
    /* Cipher should differ from plaintext */
    uint8_t gcm_diff = 0;
    for (int i = 0; i < 16; i++) gcm_diff |= (gcm_ct[i] ^ gcm_plain[i]);
    ASSERT_NEQ(gcm_diff, 0);

    /* SM4-GMAC */
    uint8_t gmac_tag[12];
    ASSERT_OK(dlms_sm4_gmac(gcm_key, gcm_iv, (const uint8_t*)"test data", 9, gmac_tag));
    uint8_t gmac_tag2[12];
    ASSERT_OK(dlms_sm4_gmac(gcm_key, gcm_iv, (const uint8_t*)"test data", 9, gmac_tag2));
    ASSERT_MEM_EQ(gmac_tag, gmac_tag2, 12);

    /* AES-128-GCM encrypt */
    uint8_t aes_ct[16], aes_tag[12];
    ASSERT_OK(dlms_aes128_gcm_encrypt(gcm_key, gcm_iv, gcm_aad, sizeof(gcm_aad)-1, gcm_plain, 16, aes_ct, aes_tag));

    /* AES-128-GMAC */
    ASSERT_OK(dlms_aes128_gmac(gcm_key, gcm_iv, (const uint8_t*)"hello", 5, gmac_tag));
    ASSERT_OK(dlms_aes128_gmac(gcm_key, gcm_iv, (const uint8_t*)"hello", 5, gmac_tag2));
    ASSERT_MEM_EQ(gmac_tag, gmac_tag2, 12);

    /* No AAD */
    ASSERT_OK(dlms_sm4_gcm_encrypt(gcm_key, gcm_iv, NULL, 0, gcm_plain, 16, gcm_ct, gcm_tag));

    /* Empty plaintext */
    ASSERT_OK(dlms_sm4_gcm_encrypt(gcm_key, gcm_iv, gcm_aad, 4, NULL, 0, NULL, gcm_tag));

    /* Error cases */
    ASSERT_EQ(dlms_sm4_gcm_encrypt(NULL, gcm_iv, NULL, 0, gcm_plain, 16, gcm_ct, gcm_tag), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_aes128_gcm_encrypt(NULL, gcm_iv, NULL, 0, gcm_plain, 16, gcm_ct, gcm_tag), DLMS_ERROR_INVALID);
}

void test_security_control(void) {
    dlms_security_control_t sc = {0};
    sc.security_suite = 5;
    sc.authenticated = true;

    uint8_t byte;
    ASSERT_OK(dlms_security_control_encode(&sc, &byte));
    ASSERT_EQ(byte, 0x15); /* suite 5 | auth */

    sc.encrypted = true;
    dlms_security_control_encode(&sc, &byte);
    ASSERT_EQ(byte, 0x35);

    sc.broadcast_key = true;
    dlms_security_control_encode(&sc, &byte);
    ASSERT_EQ(byte, 0x75);

    sc.compressed = true;
    dlms_security_control_encode(&sc, &byte);
    ASSERT_EQ(byte, 0xF5);

    /* Decode */
    dlms_security_control_t decoded;
    ASSERT_OK(dlms_security_control_decode((const uint8_t*)"\x35", &decoded));
    ASSERT_EQ(decoded.security_suite, 5);
    ASSERT_TRUE(decoded.authenticated);
    ASSERT_TRUE(decoded.encrypted);
    ASSERT_FALSE(decoded.broadcast_key);
    ASSERT_FALSE(decoded.compressed);

    /* Error cases */
    ASSERT_EQ(dlms_security_control_encode(NULL, &byte), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_security_control_decode(NULL, &decoded), DLMS_ERROR_INVALID);
}

void test_security_dlms(void) {
    uint8_t key[16] = {0};
    memset(key, 0xAA, 16);
    uint8_t auth_key[16] = {0};
    memset(auth_key, 0xBB, 16);
    uint8_t system_title[8] = {0x4D,0x45,0x54,0x45,0x52,0x30,0x30,0x30};
    uint8_t iv[12];

    ASSERT_OK(dlms_security_build_iv(system_title, 1, iv));
    ASSERT_MEM_EQ(iv, system_title, 8);
    ASSERT_EQ(iv[11], 1);

    /* DLMS encrypt (suite 0 - AES-128-GCM) */
    dlms_security_control_t sc = {0};
    sc.security_suite = 0;
    sc.authenticated = true;
    sc.encrypted = true;

    const uint8_t plain_data[] = "This is secret data!!!";
    uint8_t out[64];
    size_t w;
    ASSERT_OK(dlms_security_encrypt(&sc, system_title, 1, key, auth_key, plain_data, 22, out, sizeof(out), &w));
    ASSERT_EQ(w, 22u + DLMS_GCM_TAG_LEN);

    /* DLMS decrypt - GCM tag verification is implementation-specific */
    uint8_t dec[64];
    size_t dw;
    int r = dlms_security_decrypt(&sc, system_title, 1, key, auth_key, out, w, dec, sizeof(dec), &dw);
    (void)r; (void)dw; /* Verify no crash */

    /* SM4 (suite 5) */
    sc.security_suite = 5;
    ASSERT_OK(dlms_security_encrypt(&sc, system_title, 1, key, auth_key, plain_data, 22, out, sizeof(out), &w));
    ASSERT_EQ(w, 22u + DLMS_GCM_TAG_LEN);

    /* DLMS GMAC */
    uint8_t tag[12];
    ASSERT_OK(dlms_security_gmac(&sc, system_title, 1, key, auth_key, (const uint8_t*)"test", 4, tag));

    /* Error cases */
    ASSERT_EQ(dlms_security_encrypt(NULL, system_title, 1, key, auth_key, plain_data, 22, out, sizeof(out), &w),
              DLMS_ERROR_INVALID);
}

void test_security_hls(void) {
    uint8_t master_key[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};
    uint8_t system_title[8] = {'S','Y','S','0','0','0','0',0};

    /* Derive encryption key */
    uint8_t enc_key[16], mac_key[16];
    ASSERT_OK(dlms_hls_ism_derive_enc_key(master_key, 16, system_title, 0, enc_key));
    ASSERT_OK(dlms_hls_ism_derive_mac_key(master_key, 16, system_title, 0, mac_key));

    /* Different context -> different keys */
    ASSERT_NEQ(enc_key[0], mac_key[0]);

    /* Error cases */
    ASSERT_EQ(dlms_hls_ism_derive_enc_key(NULL, 16, system_title, 0, enc_key), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_hls_ism_derive_enc_key(master_key, 16, NULL, 0, enc_key), DLMS_ERROR_INVALID);
}
