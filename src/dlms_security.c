/**
 * @file dlms_security.c
 * @brief DLMS/COSEM Security - SM4, AES-128, GCM/GMAC, HLS-ISM
 *
 * SM4 implementation: GM/T 0002-2012 (Chinese National Standard)
 * Pure C, lookup table, zero external dependencies.
 */
#include "dlms/dlms_security.h"
#include <string.h>

/* ===== SM4 S-Box (from GM/T 0002-2012) ===== */
static const uint8_t SM4_SBOX[256] = {
    0xD6,0x90,0xE9,0xFE,0xCC,0xE1,0x3D,0xB7,0x16,0xB6,0x14,0xC2,0x28,0xFB,0x2C,0x05,
    0x2B,0x67,0x9A,0x76,0x2A,0xBE,0x04,0xC3,0xAA,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
    0x9C,0x42,0x50,0xF4,0x91,0xEF,0x98,0x7A,0x33,0x54,0x0B,0x43,0xED,0xCF,0xAC,0x62,
    0xE4,0xB3,0x1C,0xA9,0xC9,0x08,0xE8,0x95,0x80,0xDF,0x94,0xFA,0x75,0x8F,0x3F,0xA6,
    0x47,0x07,0xA7,0xFC,0xF3,0x73,0x17,0xBA,0x83,0x59,0x3C,0x19,0xE6,0x85,0x4F,0xA8,
    0x68,0x6B,0x81,0xB2,0x71,0x64,0xDA,0x8B,0xF8,0xEB,0x0F,0x4B,0x70,0x56,0x9D,0x35,
    0x1E,0x24,0x0E,0x5E,0x63,0x58,0xD1,0xA2,0x25,0x22,0x7C,0x3B,0x01,0x21,0x78,0x87,
    0xD4,0x00,0x46,0x57,0x9F,0xD3,0x27,0x52,0x4C,0x36,0x02,0xE7,0xA0,0xC4,0xC8,0x9E,
    0xEA,0xBF,0x8A,0xD2,0x40,0xC7,0x38,0xB5,0xA3,0xF7,0xF2,0xCE,0xF9,0x61,0x15,0xA1,
    0xE0,0xAE,0x5D,0xA4,0x9B,0x34,0x1A,0x55,0xAD,0x93,0x32,0x30,0xF5,0x8C,0xB1,0xE3,
    0x1D,0xF6,0xE2,0x2E,0x82,0x66,0xCA,0x60,0xC0,0x29,0x23,0xAB,0x0D,0x53,0x4E,0x6F,
    0xD5,0xDB,0x37,0x45,0xDE,0xFD,0x8E,0x2F,0x03,0xFF,0x6A,0x72,0x6D,0x6C,0x5B,0x51,
    0x8D,0x1B,0xAF,0x92,0xBB,0xDD,0xBC,0x7F,0x11,0xD9,0x5C,0x41,0x1F,0x10,0x5A,0xD8,
    0x0A,0xC1,0x31,0x88,0xA5,0xCD,0x7B,0xBD,0x2D,0x74,0xD0,0x12,0xB8,0xE5,0xB4,0xB0,
    0x89,0x69,0x97,0x4A,0x0C,0x96,0x77,0x7E,0x65,0xB9,0xF1,0x09,0xC5,0x6E,0xC6,0x84,
    0x18,0xF0,0x7D,0xEC,0x3A,0xDC,0x4D,0x20,0x79,0xEE,0x5F,0x3E,0xD7,0xCB,0x39,0x48
};

/* ===== SM4 CK constant ===== */
static const uint32_t SM4_CK[32] = {
    0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
    0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
    0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
    0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
    0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
    0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
    0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
    0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
};

/* ===== SM4 FK constant ===== */
static const uint32_t SM4_FK[4] = { 0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC };

/* ===== SM4 internal helpers ===== */

static uint8_t sm4_sbox(uint8_t b) { return SM4_SBOX[b]; }

static uint32_t sm4_tau(uint32_t x) {
    uint8_t a = sm4_sbox((uint8_t)(x >> 24));
    uint8_t b = sm4_sbox((uint8_t)(x >> 16));
    uint8_t c = sm4_sbox((uint8_t)(x >> 8));
    uint8_t d = sm4_sbox((uint8_t)(x));
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
}

static uint32_t sm4_L(uint32_t x) {
    return (x ^ ((x << 2) | (x >> 30)) ^ ((x << 10) | (x >> 22)) ^
            ((x << 18) | (x >> 14)) ^ ((x << 24) | (x >> 8)));
}

static uint32_t sm4_L_prime(uint32_t x) {
    return (x ^ ((x << 13) | (x >> 19)) ^ ((x << 23) | (x >> 9)));
}

static uint32_t sm4_T(uint32_t x) { return sm4_L(sm4_tau(x)); }
static uint32_t sm4_T_prime(uint32_t x) { return sm4_L_prime(sm4_tau(x)); }

/* ===== SM4 Key Setup ===== */

void dlms_sm4_key_setup(dlms_sm4_ctx_t *ctx, const uint8_t key[SM4_KEY_SIZE]) {
    uint32_t MK[4], K[36];
    for (int i = 0; i < 4; i++) {
        MK[i] = ((uint32_t)key[4*i] << 24) | ((uint32_t)key[4*i+1] << 16) |
                ((uint32_t)key[4*i+2] << 8) | key[4*i+3];
        K[i] = MK[i] ^ SM4_FK[i];
    }
    for (int i = 0; i < 32; i++) {
        K[i+4] = K[i] ^ sm4_T_prime(K[i+1] ^ K[i+2] ^ K[i+3] ^ SM4_CK[i]);
        ctx->rk[i] = K[i+4];
    }
}

/* ===== SM4 Block Encrypt/Decrypt ===== */

static void sm4_round(const uint32_t rk[32], const uint8_t in[16], uint8_t out[16]) {
    uint32_t x[4];
    for (int i = 0; i < 4; i++)
        x[i] = ((uint32_t)in[4*i] << 24) | ((uint32_t)in[4*i+1] << 16) |
               ((uint32_t)in[4*i+2] << 8) | in[4*i+3];

    for (int i = 0; i < 32; i++) {
        uint32_t tmp = x[1] ^ x[2] ^ x[3] ^ rk[i];
        tmp = sm4_T(tmp);
        x[0] = x[0] ^ tmp;
        /* Rotate */
        uint32_t t = x[0];
        x[0] = x[1]; x[1] = x[2]; x[2] = x[3]; x[3] = t;
    }

    /* Reverse */
    for (int i = 0; i < 4; i++) {
        uint32_t v = x[3 - i];
        out[4*i]   = (uint8_t)(v >> 24);
        out[4*i+1] = (uint8_t)(v >> 16);
        out[4*i+2] = (uint8_t)(v >> 8);
        out[4*i+3] = (uint8_t)(v);
    }
}

void dlms_sm4_encrypt_block(const dlms_sm4_ctx_t *ctx, const uint8_t in[SM4_BLOCK_SIZE], uint8_t out[SM4_BLOCK_SIZE]) {
    sm4_round(ctx->rk, in, out);
}

void dlms_sm4_decrypt_block(const dlms_sm4_ctx_t *ctx, const uint8_t in[SM4_BLOCK_SIZE], uint8_t out[SM4_BLOCK_SIZE]) {
    uint32_t rk_rev[32];
    for (int i = 0; i < 32; i++) rk_rev[i] = ctx->rk[31 - i];
    sm4_round(rk_rev, in, out);
}

/* ===== SM4-ECB ===== */

int dlms_sm4_ecb_encrypt(const uint8_t *key, const uint8_t *in, size_t len, uint8_t *out) {
    if (!key || !in || !out || len % 16 != 0) return DLMS_ERROR_INVALID;
    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);
    for (size_t i = 0; i < len; i += 16)
        dlms_sm4_encrypt_block(&ctx, in + i, out + i);
    return DLMS_OK;
}

int dlms_sm4_ecb_decrypt(const uint8_t *key, const uint8_t *in, size_t len, uint8_t *out) {
    if (!key || !in || !out || len % 16 != 0) return DLMS_ERROR_INVALID;
    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);
    for (size_t i = 0; i < len; i += 16)
        dlms_sm4_decrypt_block(&ctx, in + i, out + i);
    return DLMS_OK;
}

/* ===== SM4-CBC ===== */

int dlms_sm4_cbc_encrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *in, size_t len, uint8_t *out) {
    if (!key || !iv || !in || !out || len % 16 != 0) return DLMS_ERROR_INVALID;
    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);
    uint8_t block[16];
    memcpy(block, iv, 16);
    for (size_t i = 0; i < len; i += 16) {
        for (int j = 0; j < 16; j++) block[j] ^= in[i + j];
        dlms_sm4_encrypt_block(&ctx, block, out + i);
        memcpy(block, out + i, 16);
    }
    return DLMS_OK;
}

int dlms_sm4_cbc_decrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *in, size_t len, uint8_t *out) {
    if (!key || !iv || !in || !out || len % 16 != 0) return DLMS_ERROR_INVALID;
    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);
    uint8_t prev[16];
    memcpy(prev, iv, 16);
    for (size_t i = 0; i < len; i += 16) {
        uint8_t tmp[16];
        dlms_sm4_decrypt_block(&ctx, in + i, tmp);
        for (int j = 0; j < 16; j++) out[i + j] = tmp[j] ^ prev[j];
        memcpy(prev, in + i, 16);
    }
    return DLMS_OK;
}

/* ===== SM4-CTR ===== */

int dlms_sm4_ctr(const uint8_t *key, const uint8_t *nonce,
                 const uint8_t *in, size_t len, uint8_t *out) {
    if (!key || !nonce || !in || !out) return DLMS_ERROR_INVALID;
    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);
    uint8_t counter[16];
    memset(counter, 0, 16);
    memcpy(counter, nonce, 12); /* 96-bit nonce */
    /* Counter starts at 1 (big-endian in last 4 bytes) */
    counter[15] = 1;
    for (size_t i = 0; i < len; i += 16) {
        uint8_t keystream[16];
        dlms_sm4_encrypt_block(&ctx, counter, keystream);
        size_t block_len = (len - i < 16) ? len - i : 16;
        for (size_t j = 0; j < block_len; j++)
            out[i + j] = in[i + j] ^ keystream[j];
        /* Increment counter */
        for (int k = 15; k >= 12; k--) {
            if (++counter[k] != 0) break;
        }
    }
    return DLMS_OK;
}

/* ===== SM4-GCM internal ===== */

/* GHASH: GF(2^128) multiplication */
static void sm4_ghash(const uint8_t *H, const uint8_t *data, size_t data_len, uint8_t *out) {
    uint8_t Y[16];
    memset(Y, 0, 16);

    for (size_t i = 0; i < data_len; i += 16) {
        for (int j = 0; j < 16; j++) Y[j] ^= data[i + j];

        /* GF(2^128) multiplication Y * H */
        uint8_t V[16];
        memset(V, 0, 16);
        for (int bit = 0; bit < 128; bit++) {
            /* V = V << 1 (in GF(2^128)) */
            uint8_t carry = V[0] >> 7;
            for (int k = 0; k < 15; k++) V[k] = (V[k] << 1) | (V[k+1] >> 7);
            V[15] = V[15] << 1;
            if (carry) V[15] ^= 0xE1; /* R = 11100001 */

            if (Y[bit / 8] & (0x80 >> (bit % 8))) {
                for (int k = 0; k < 16; k++) V[k] ^= H[k];
            }
        }
        memcpy(Y, V, 16);
    }
    memcpy(out, Y, 16);
}

/* GCTR (counter mode with GCM increment) */
static void sm4_gctr(const dlms_sm4_ctx_t *ctx, const uint8_t *icb,
                     const uint8_t *in, size_t len, uint8_t *out) {
    uint8_t counter[16];
    memcpy(counter, icb, 16);
    for (size_t i = 0; i < len; i += 16) {
        uint8_t keystream[16];
        dlms_sm4_encrypt_block(ctx, counter, keystream);
        size_t block_len = (len - i < 16) ? len - i : 16;
        for (size_t j = 0; j < block_len; j++)
            out[i + j] = in[i + j] ^ keystream[j];
        /* Increment counter (last 4 bytes) */
        for (int k = 15; k >= 12; k--) {
            if (++counter[k] != 0) break;
        }
    }
}

/* GCM inc32 */
static void gcm_inc32(const uint8_t *in, uint8_t *out) {
    memcpy(out, in, 16);
    for (int i = 15; i >= 12; i--) {
        if (++out[i] != 0) break;
    }
}

int dlms_sm4_gcm_encrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *plain, size_t plain_len,
                         uint8_t *cipher, uint8_t *tag) {
    if (!key || !iv || !tag) return DLMS_ERROR_INVALID;

    dlms_sm4_ctx_t ctx;
    dlms_sm4_key_setup(&ctx, key);

    /* H = E_K(0^128) */
    uint8_t H[16], zero_block[16];
    memset(zero_block, 0, 16);
    dlms_sm4_encrypt_block(&ctx, zero_block, H);

    /* J0 = IV || 0^31 || 1 (when IV is 96-bit) */
    uint8_t J0[16];
    memset(J0, 0, 16);
    memcpy(J0, iv, 12);
    J0[15] = 1;

    /* Encrypt */
    uint8_t ICB[16];
    gcm_inc32(J0, ICB);
    if (plain_len > 0 && cipher) {
        sm4_gctr(&ctx, ICB, plain, plain_len, cipher);
    }

    /* GHASH */
    /* Build input: AAD || pad || ciphertext || pad || len(AAD) || len(CT) */
    uint8_t ghash_input[1024];
    size_t gi = 0;

    /* AAD */
    if (aad_len > 0 && aad) {
        memcpy(ghash_input + gi, aad, aad_len);
        gi += aad_len;
    }
    /* Pad AAD to 16-byte boundary */
    while (gi % 16 != 0) ghash_input[gi++] = 0;

    /* Ciphertext */
    const uint8_t *ct = cipher ? cipher : plain;
    memcpy(ghash_input + gi, ct, plain_len);
    gi += plain_len;
    while (gi % 16 != 0) ghash_input[gi++] = 0;

    /* Length block: len(AAD) || len(CT) in bits (64-bit each, big-endian) */
    uint8_t len_block[16];
    memset(len_block, 0, 16);
    uint64_t aad_bits = (uint64_t)aad_len * 8;
    uint64_t ct_bits = (uint64_t)plain_len * 8;
    for (int i = 0; i < 8; i++) {
        len_block[i] = (uint8_t)(aad_bits >> (56 - 8*i));
        len_block[8+i] = (uint8_t)(ct_bits >> (56 - 8*i));
    }
    memcpy(ghash_input + gi, len_block, 16);
    gi += 16;

    uint8_t S[16];
    sm4_ghash(H, ghash_input, gi, S);

    /* Tag = GCTR_K(J0, S) */
    uint8_t full_tag[16];
    sm4_gctr(&ctx, J0, S, 16, full_tag);
    memcpy(tag, full_tag, DLMS_GCM_TAG_LEN);

    return DLMS_OK;
}

int dlms_sm4_gcm_decrypt(const uint8_t *key, const uint8_t *iv,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *cipher, size_t cipher_len,
                         const uint8_t *tag, uint8_t *plain) {
    if (!key || !iv || !tag || !cipher || !plain) return DLMS_ERROR_INVALID;

    /* Verify tag first */
    uint8_t calc_tag[DLMS_GCM_TAG_LEN];
    int ret = dlms_sm4_gcm_encrypt(key, iv, aad, aad_len, cipher, cipher_len, plain, calc_tag);
    if (ret != DLMS_OK) return ret;
    if (memcmp(calc_tag, tag, DLMS_GCM_TAG_LEN) != 0) return DLMS_ERROR;

    return DLMS_OK;
}

int dlms_sm4_gmac(const uint8_t *key, const uint8_t *iv,
                  const uint8_t *data, size_t len, uint8_t *tag) {
    return dlms_sm4_gcm_encrypt(key, iv, data, len, NULL, 0, NULL, tag);
}

/* ===== AES-128 (simplified implementation for suites 0-4) ===== */

/* AES S-Box */
static const uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t AES_RCON[11] = { 0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36 };

static uint32_t aes_sub_word(uint32_t w) {
    return ((uint32_t)AES_SBOX[(w>>24)&0xFF]<<24) | ((uint32_t)AES_SBOX[(w>>16)&0xFF]<<16) |
           ((uint32_t)AES_SBOX[(w>>8)&0xFF]<<8) | AES_SBOX[w&0xFF];
}

static uint32_t aes_rot_word(uint32_t w) { return (w<<8)|(w>>24); }

void dlms_aes128_key_setup(dlms_aes128_ctx_t *ctx, const uint8_t key[AES128_KEY_SIZE]) {
    uint32_t w[44];
    for (int i = 0; i < 4; i++)
        w[i] = ((uint32_t)key[4*i]<<24)|((uint32_t)key[4*i+1]<<16)|((uint32_t)key[4*i+2]<<8)|key[4*i+3];
    for (int i = 4; i < 44; i++) {
        uint32_t temp = w[i-1];
        if (i % 4 == 0) temp = aes_sub_word(aes_rot_word(temp)) ^ ((uint32_t)AES_RCON[i/4]<<24);
        w[i] = w[i-4] ^ temp;
    }
    memcpy(ctx->rk, w, sizeof(w));
}

static void aes_add_round_key(uint8_t state[16], const uint32_t *rk, int round) {
    for (int i = 0; i < 4; i++) {
        uint32_t k = rk[round * 4 + i];
        state[4*i]   ^= (uint8_t)(k>>24);
        state[4*i+1] ^= (uint8_t)(k>>16);
        state[4*i+2] ^= (uint8_t)(k>>8);
        state[4*i+3] ^= k;
    }
}

static void aes_sub_bytes(uint8_t s[16]) { for (int i = 0; i < 16; i++) s[i] = AES_SBOX[s[i]]; }

static void aes_shift_rows(uint8_t s[16]) {
    uint8_t t;
    /* Row 1: shift left 1 */
    t=s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
    /* Row 2: shift left 2 */
    t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
    /* Row 3: shift left 3 */
    t=s[13]; s[13]=s[9]; s[9]=s[5]; s[5]=s[1]; s[1]=t;
    /* Fix row 3 */
    t=s[3]; s[3]=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=t;
}

static uint8_t aes_xtime(uint8_t a) { return (a<<1)^((a>>7)&1)*0x1b; }

static void aes_mix_columns(uint8_t s[16]) {
    for (int i = 0; i < 4; i++) {
        uint8_t a0=s[4*i],a1=s[4*i+1],a2=s[4*i+2],a3=s[4*i+3];
        s[4*i]   = aes_xtime(a0)^aes_xtime(a1)^a1^a2^a3;
        s[4*i+1] = a0^aes_xtime(a1)^aes_xtime(a2)^a2^a3;
        s[4*i+2] = a0^a1^aes_xtime(a2)^aes_xtime(a3)^a3;
        s[4*i+3] = aes_xtime(a0)^a0^a1^a2^aes_xtime(a3);
    }
}

void dlms_aes128_encrypt_block(const dlms_aes128_ctx_t *ctx, const uint8_t in[AES128_BLOCK_SIZE], uint8_t out[AES128_BLOCK_SIZE]) {
    uint8_t state[16];
    memcpy(state, in, 16);
    aes_add_round_key(state, ctx->rk, 0);
    for (int r = 1; r <= 9; r++) {
        aes_sub_bytes(state);
        aes_shift_rows(state);
        aes_mix_columns(state);
        aes_add_round_key(state, ctx->rk, r);
    }
    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_add_round_key(state, ctx->rk, 10);
    memcpy(out, state, 16);
}

/* AES-128-GCM (reuses SM4 GHASH since it's algorithm-agnostic) */
int dlms_aes128_gcm_encrypt(const uint8_t *key, const uint8_t *iv,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *plain, size_t plain_len,
                            uint8_t *cipher, uint8_t *tag) {
    if (!key || !iv || !tag) return DLMS_ERROR_INVALID;

    dlms_aes128_ctx_t ctx;
    dlms_aes128_key_setup(&ctx, key);

    uint8_t H[16], zero[16];
    memset(zero, 0, 16);
    dlms_aes128_encrypt_block(&ctx, zero, H);

    uint8_t J0[16];
    memset(J0, 0, 16);
    memcpy(J0, iv, 12);
    J0[15] = 1;

    uint8_t ICB[16];
    gcm_inc32(J0, ICB);

    if (plain_len > 0 && cipher) {
        sm4_gctr((const dlms_sm4_ctx_t *)&ctx, ICB, plain, plain_len, cipher);
    }

    uint8_t ghash_input[1024];
    size_t gi = 0;
    if (aad_len > 0 && aad) { memcpy(ghash_input+gi, aad, aad_len); gi += aad_len; }
    while (gi % 16 != 0) ghash_input[gi++] = 0;
    const uint8_t *ct = cipher ? cipher : plain;
    memcpy(ghash_input+gi, ct, plain_len); gi += plain_len;
    while (gi % 16 != 0) ghash_input[gi++] = 0;
    uint8_t len_block[16]; memset(len_block, 0, 16);
    uint64_t ab=(uint64_t)aad_len*8, cb=(uint64_t)plain_len*8;
    for (int i=0;i<8;i++) { len_block[i]=(uint8_t)(ab>>(56-8*i)); len_block[8+i]=(uint8_t)(cb>>(56-8*i)); }
    memcpy(ghash_input+gi, len_block, 16); gi += 16;

    uint8_t S[16]; sm4_ghash(H, ghash_input, gi, S);
    uint8_t full_tag[16]; sm4_gctr((const dlms_sm4_ctx_t *)&ctx, J0, S, 16, full_tag);
    memcpy(tag, full_tag, DLMS_GCM_TAG_LEN);
    return DLMS_OK;
}

int dlms_aes128_gcm_decrypt(const uint8_t *key, const uint8_t *iv,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *cipher, size_t cipher_len,
                            const uint8_t *tag, uint8_t *plain) {
    uint8_t calc_tag[DLMS_GCM_TAG_LEN];
    int ret = dlms_aes128_gcm_encrypt(key, iv, aad, aad_len, cipher, cipher_len, plain, calc_tag);
    if (ret != DLMS_OK) return ret;
    if (memcmp(calc_tag, tag, DLMS_GCM_TAG_LEN) != 0) return DLMS_ERROR;
    return DLMS_OK;
}

int dlms_aes128_gmac(const uint8_t *key, const uint8_t *iv,
                     const uint8_t *data, size_t len, uint8_t *tag) {
    return dlms_aes128_gcm_encrypt(key, iv, data, len, NULL, 0, NULL, tag);
}

/* ===== Security Control ===== */

int dlms_security_control_encode(const dlms_security_control_t *sc, uint8_t *dst) {
    if (!sc || !dst) return DLMS_ERROR_INVALID;
    uint8_t b = sc->security_suite & 0x0F;
    if (sc->authenticated) b |= 0x10;
    if (sc->encrypted) b |= 0x20;
    if (sc->broadcast_key) b |= 0x40;
    if (sc->compressed) b |= 0x80;
    *dst = b;
    return DLMS_OK;
}

int dlms_security_control_decode(const uint8_t *data, dlms_security_control_t *sc) {
    if (!data || !sc) return DLMS_ERROR_INVALID;
    uint8_t b = *data;
    sc->security_suite = b & 0x0F;
    sc->authenticated = (b & 0x10) != 0;
    sc->encrypted = (b & 0x20) != 0;
    sc->broadcast_key = (b & 0x40) != 0;
    sc->compressed = (b & 0x80) != 0;
    return DLMS_OK;
}

int dlms_security_build_iv(const uint8_t *system_title, uint32_t invocation_counter, uint8_t *iv) {
    if (!system_title || !iv) return DLMS_ERROR_INVALID;
    memcpy(iv, system_title, 8);
    iv[8]  = (uint8_t)(invocation_counter >> 24);
    iv[9]  = (uint8_t)(invocation_counter >> 16);
    iv[10] = (uint8_t)(invocation_counter >> 8);
    iv[11] = (uint8_t)(invocation_counter);
    return DLMS_OK;
}

/* ===== DLMS-level security ===== */

int dlms_security_encrypt(const dlms_security_control_t *sc,
                          const uint8_t *system_title, uint32_t invocation_counter,
                          const uint8_t *enc_key, const uint8_t *auth_key,
                          const uint8_t *plain, size_t plain_len,
                          uint8_t *out, size_t out_size, size_t *written) {
    if (!sc || !system_title || !enc_key || !auth_key || !out || !written)
        return DLMS_ERROR_INVALID;
    if (out_size < plain_len + DLMS_GCM_TAG_LEN) return DLMS_ERROR_BUFFER;

    uint8_t iv[12];
    dlms_security_build_iv(system_title, invocation_counter, iv);

    uint8_t sc_byte;
    dlms_security_control_encode(sc, &sc_byte);

    /* AAD = security_control + auth_key */
    uint8_t aad[33];
    aad[0] = sc_byte;
    uint8_t ak_len = (sc->security_suite >= 2 && sc->security_suite <= 4) ? 32 : 16;
    memcpy(aad + 1, auth_key, ak_len);

    int ret;
    if (sc->security_suite == 5) {
        ret = dlms_sm4_gcm_encrypt(enc_key, iv, aad, 1 + ak_len, plain, plain_len,
                                   out, out + plain_len);
    } else {
        ret = dlms_aes128_gcm_encrypt(enc_key, iv, aad, 1 + ak_len, plain, plain_len,
                                      out, out + plain_len);
    }
    if (ret != DLMS_OK) return ret;
    *written = plain_len + DLMS_GCM_TAG_LEN;
    return DLMS_OK;
}

int dlms_security_decrypt(const dlms_security_control_t *sc,
                          const uint8_t *system_title, uint32_t invocation_counter,
                          const uint8_t *enc_key, const uint8_t *auth_key,
                          const uint8_t *cipher, size_t cipher_len,
                          uint8_t *plain, size_t plain_size, size_t *written) {
    if (!sc || !system_title || !enc_key || !auth_key || !cipher || !plain || !written)
        return DLMS_ERROR_INVALID;
    if (cipher_len < DLMS_GCM_TAG_LEN) return DLMS_ERROR_INVALID;

    size_t ct_len = cipher_len - DLMS_GCM_TAG_LEN;
    if (plain_size < ct_len) return DLMS_ERROR_BUFFER;

    uint8_t iv[12];
    dlms_security_build_iv(system_title, invocation_counter, iv);

    uint8_t sc_byte;
    dlms_security_control_encode(sc, &sc_byte);

    uint8_t aad[33];
    aad[0] = sc_byte;
    uint8_t ak_len = (sc->security_suite >= 2 && sc->security_suite <= 4) ? 32 : 16;
    memcpy(aad + 1, auth_key, ak_len);

    int ret;
    if (sc->security_suite == 5) {
        ret = dlms_sm4_gcm_decrypt(enc_key, iv, aad, 1 + ak_len, cipher, ct_len,
                                   cipher + ct_len, plain);
    } else {
        ret = dlms_aes128_gcm_decrypt(enc_key, iv, aad, 1 + ak_len, cipher, ct_len,
                                      cipher + ct_len, plain);
    }
    if (ret != DLMS_OK) return ret;
    *written = ct_len;
    return DLMS_OK;
}

int dlms_security_gmac(const dlms_security_control_t *sc,
                       const uint8_t *system_title, uint32_t invocation_counter,
                       const uint8_t *enc_key, const uint8_t *auth_key,
                       const uint8_t *data, size_t data_len,
                       uint8_t *tag) {
    if (!sc || !system_title || !enc_key || !auth_key || !tag) return DLMS_ERROR_INVALID;

    uint8_t iv[12];
    dlms_security_build_iv(system_title, invocation_counter, iv);

    uint8_t sc_byte;
    dlms_security_control_encode(sc, &sc_byte);

    /* AAD = security_control + auth_key + data */
    uint8_t aad[256 + 33];
    size_t aad_len = 0;
    aad[aad_len++] = sc_byte;
    uint8_t ak_len = (sc->security_suite >= 2 && sc->security_suite <= 4) ? 32 : 16;
    memcpy(aad + aad_len, auth_key, ak_len);
    aad_len += ak_len;
    if (data_len > 0 && data) {
        if (aad_len + data_len > sizeof(aad)) return DLMS_ERROR_BUFFER;
        memcpy(aad + aad_len, data, data_len);
        aad_len += data_len;
    }

    if (sc->security_suite == 5) {
        return dlms_sm4_gmac(enc_key, iv, aad, aad_len, tag);
    } else {
        return dlms_aes128_gmac(enc_key, iv, aad, aad_len, tag);
    }
}

/* ===== HLS-ISM (simplified - uses GMAC directly) ===== */

int dlms_hls_ism_derive_key(const uint8_t *master_key, uint16_t master_key_len,
                            const uint8_t *context, uint16_t context_len,
                            uint8_t *derived_key, uint16_t derived_key_len) {
    /* Simplified KDF using AES-CMAC-like construction.
     * For production, implement proper HMAC-SHA256 based KDF. */
    (void)master_key_len;
    if (!master_key || !context || !derived_key) return DLMS_ERROR_INVALID;
    if (derived_key_len > 32) return DLMS_ERROR_INVALID;

    /* Simple XOR-based derivation for embedded use */
    uint8_t counter = 1;
    for (uint16_t i = 0; i < derived_key_len; i++) {
        uint8_t ki = i % 16;
        uint8_t ci = (i + counter - 1) % context_len;
        derived_key[i] = master_key[ki] ^ context[ci] ^ counter;
    }
    return DLMS_OK;
}

int dlms_hls_ism_derive_enc_key(const uint8_t *master_key, uint16_t key_len,
                                const uint8_t *system_title, uint8_t suite, uint8_t *enc_key) {
    if (!master_key || !system_title || !enc_key) return DLMS_ERROR_INVALID;
    uint8_t context[11] = {'E','N','C'};
    memcpy(context + 3, system_title, 8);
    uint8_t dk_len = (suite == 2 || suite == 4) ? 32 : 16;
    return dlms_hls_ism_derive_key(master_key, key_len, context, 11, enc_key, dk_len);
}

int dlms_hls_ism_derive_mac_key(const uint8_t *master_key, uint16_t key_len,
                                const uint8_t *system_title, uint8_t suite, uint8_t *mac_key) {
    if (!master_key || !system_title || !mac_key) return DLMS_ERROR_INVALID;
    uint8_t context[11] = {'M','A','C'};
    memcpy(context + 3, system_title, 8);
    uint8_t dk_len = (suite == 2 || suite == 4) ? 32 : 16;
    return dlms_hls_ism_derive_key(master_key, key_len, context, 11, mac_key, dk_len);
}

/* ===== AES Key Wrap (RFC 3394 simplified) ===== */

int dlms_aes_key_wrap(const uint8_t *kek, const uint8_t *key_to_wrap,
                      uint16_t key_len, uint8_t *wrapped, uint16_t *wrapped_len) {
    (void)kek; (void)key_to_wrap; (void)key_len; (void)wrapped;
    if (wrapped_len) *wrapped_len = 0;
    return DLMS_ERROR_NOT_SUPPORTED; /* TODO: implement */
}

int dlms_aes_key_unwrap(const uint8_t *kek, const uint8_t *wrapped,
                        uint16_t wrapped_len, uint8_t *unwrapped, uint16_t *key_len) {
    (void)kek; (void)wrapped; (void)wrapped_len; (void)unwrapped;
    if (key_len) *key_len = 0;
    return DLMS_ERROR_NOT_SUPPORTED; /* TODO: implement */
}
