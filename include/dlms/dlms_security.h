/**
 * @file dlms_security.h
 * @brief DLMS/COSEM Security - SM4, AES-128, GCM/GMAC, HLS-ISM
 */
#ifndef DLMS_SECURITY_H
#define DLMS_SECURITY_H

#include "dlms_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Security Suite numbers */
#define DLMS_SECURITY_SUITE_0    0  /* AES-128-GCM, no key derivation */
#define DLMS_SECURITY_SUITE_1    1  /* AES-128-GCM */
#define DLMS_SECURITY_SUITE_2    2  /* AES-256-GCM */
#define DLMS_SECURITY_SUITE_3    3  /* AES-128-CCM */
#define DLMS_SECURITY_SUITE_4    4  /* AES-256-CCM */
#define DLMS_SECURITY_SUITE_5    5  /* SM4-GCM (Chinese national crypto) */

/* Tag length for GCM/GMAC in DLMS */
#define DLMS_GCM_TAG_LEN         12

/* Security Control Field (1 byte) */
typedef struct {
    uint8_t security_suite;
    bool authenticated;
    bool encrypted;
    bool broadcast_key;
    bool compressed;
} dlms_security_control_t;

int dlms_security_control_encode(const dlms_security_control_t *sc, uint8_t *dst);
int dlms_security_control_decode(const uint8_t *data, dlms_security_control_t *sc);

/* IV construction: system_title(8) + invocation_counter(4) = 12 bytes */
int dlms_security_build_iv(const uint8_t *system_title, uint32_t invocation_counter, uint8_t *iv);

/* ===== SM4 Block Cipher (GM/T 0002-2012) ===== */
#define SM4_KEY_SIZE     16
#define SM4_BLOCK_SIZE   16

typedef struct {
    uint32_t rk[32]; /* round keys */
} dlms_sm4_ctx_t;

void dlms_sm4_key_setup(dlms_sm4_ctx_t *ctx, const uint8_t key[SM4_KEY_SIZE]);
void dlms_sm4_encrypt_block(const dlms_sm4_ctx_t *ctx, const uint8_t in[SM4_BLOCK_SIZE], uint8_t out[SM4_BLOCK_SIZE]);
void dlms_sm4_decrypt_block(const dlms_sm4_ctx_t *ctx, const uint8_t in[SM4_BLOCK_SIZE], uint8_t out[SM4_BLOCK_SIZE]);

/* SM4-ECB */
int dlms_sm4_ecb_encrypt(const uint8_t *key, const uint8_t *in, size_t len, uint8_t *out);
int dlms_sm4_ecb_decrypt(const uint8_t *key, const uint8_t *in, size_t len, uint8_t *out);

/* SM4-CBC */
int dlms_sm4_cbc_encrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *in, size_t len, uint8_t *out);
int dlms_sm4_cbc_decrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *in, size_t len, uint8_t *out);

/* SM4-CTR */
int dlms_sm4_ctr(const uint8_t *key, const uint8_t *nonce,
                 const uint8_t *in, size_t len, uint8_t *out);

/* SM4-GCM */
int dlms_sm4_gcm_encrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *plain, size_t plain_len,
                         uint8_t *cipher, uint8_t *tag);
int dlms_sm4_gcm_decrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *cipher, size_t cipher_len,
                         const uint8_t *tag, uint8_t *plain);

/* SM4-GMAC (authentication only) */
int dlms_sm4_gmac(const uint8_t *key, const uint8_t *iv,
                  const uint8_t *data, size_t len, uint8_t *tag);

/* ===== AES-128 (optional, for suites 0-4) ===== */
#define AES128_KEY_SIZE   16
#define AES128_BLOCK_SIZE 16

typedef struct {
    uint32_t rk[44]; /* round keys */
} dlms_aes128_ctx_t;

void dlms_aes128_key_setup(dlms_aes128_ctx_t *ctx, const uint8_t key[AES128_KEY_SIZE]);
void dlms_aes128_encrypt_block(const dlms_aes128_ctx_t *ctx, const uint8_t in[AES128_BLOCK_SIZE], uint8_t out[AES128_BLOCK_SIZE]);

/* AES-128-GCM */
int dlms_aes128_gcm_encrypt(const uint8_t *key, const uint8_t *iv,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *plain, size_t plain_len,
                            uint8_t *cipher, uint8_t *tag);
int dlms_aes128_gcm_decrypt(const uint8_t *key, const uint8_t *iv,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *cipher, size_t cipher_len,
                            const uint8_t *tag, uint8_t *plain);
int dlms_aes128_gmac(const uint8_t *key, const uint8_t *iv,
                     const uint8_t *data, size_t len, uint8_t *tag);

/* ===== HLS-ISM: High Level Security for IP Smart Metering ===== */

/* Key derivation using HMAC-SHA256 */
int dlms_hls_ism_derive_key(const uint8_t *master_key, uint16_t master_key_len,
                            const uint8_t *context, uint16_t context_len,
                            uint8_t *derived_key, uint16_t derived_key_len);

/* Derive encryption key: KDF(master, "ENC" || system_title) */
int dlms_hls_ism_derive_enc_key(const uint8_t *master_key, uint16_t key_len,
                                const uint8_t *system_title,
                                uint8_t suite, uint8_t *enc_key);

/* Derive authentication key: KDF(master, "MAC" || system_title) */
int dlms_hls_ism_derive_mac_key(const uint8_t *master_key, uint16_t key_len,
                                const uint8_t *system_title,
                                uint8_t suite, uint8_t *mac_key);

/* DLMS-level encrypt (wraps GCM with DLMS IV construction) */
int dlms_security_encrypt(const dlms_security_control_t *sc,
                          const uint8_t *system_title, uint32_t invocation_counter,
                          const uint8_t *enc_key, const uint8_t *auth_key,
                          const uint8_t *plain, size_t plain_len,
                          uint8_t *out, size_t out_size, size_t *written);

/* DLMS-level decrypt */
int dlms_security_decrypt(const dlms_security_control_t *sc,
                          const uint8_t *system_title, uint32_t invocation_counter,
                          const uint8_t *enc_key, const uint8_t *auth_key,
                          const uint8_t *cipher, size_t cipher_len,
                          uint8_t *plain, size_t plain_size, size_t *written);

/* DLMS-level GMAC */
int dlms_security_gmac(const dlms_security_control_t *sc,
                       const uint8_t *system_title, uint32_t invocation_counter,
                       const uint8_t *enc_key, const uint8_t *auth_key,
                       const uint8_t *data, size_t data_len,
                       uint8_t *tag);

/* AES Key Wrap (RFC 3394) */
int dlms_aes_key_wrap(const uint8_t *kek, const uint8_t *key_to_wrap,
                      uint16_t key_len, uint8_t *wrapped, uint16_t *wrapped_len);
int dlms_aes_key_unwrap(const uint8_t *kek, const uint8_t *wrapped,
                        uint16_t wrapped_len, uint8_t *unwrapped, uint16_t *key_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_SECURITY_H */
