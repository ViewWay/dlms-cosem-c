#include "dlms/dlms_client.h"
#include <stdio.h>
#include <string.h>

/**
 * Example: Simulate a DLMS meter connection.
 * This demonstrates the API usage pattern without real hardware.
 */

/* Mock IO: send to buffer */
static uint8_t mock_send_buf[512];
static size_t mock_send_len = 0;

static int mock_send(const uint8_t *data, size_t len, void *user_data) {
    (void)user_data;
    if (len > sizeof(mock_send_buf)) return -1;
    memcpy(mock_send_buf, data, len);
    mock_send_len = len;
    printf("  TX: %zu bytes\n", len);
    for (size_t i = 0; i < len && i < 32; i++) printf(" %02X", data[i]);
    printf("\n");
    return 0;
}

static int mock_recv(uint8_t *data, size_t max_len, size_t *received, int timeout_ms, void *user_data) {
    (void)user_data; (void)timeout_ms;
    /* Build a UA response frame */
    dlms_hdlc_address_t dest = { .logical_address = 0x10, .has_physical = false };
    dlms_hdlc_address_t src = { .logical_address = 1, .has_physical = false };
    int len = dlms_hdlc_build_ua(data, max_len, &dest, &src, NULL);
    if (len > 0) { *received = (size_t)len; return 0; }
    return -1;
}

int main(void) {
    printf("=== DLMS/COSEM Meter Simulation Example ===\n\n");

    /* Initialize client */
    dlms_client_t client;
    if (dlms_client_init(&client) != DLMS_OK) {
        printf("Failed to init client\n");
        return 1;
    }

    /* Set mock IO */
    dlms_client_set_io(&client, mock_send, mock_recv, NULL);

    /* Set addresses */
    dlms_client_set_addresses(&client, 0x10, 0x01);

    printf("1. Testing HDLC connect...\n");
    int ret = dlms_hdlc_connect(&client);
    if (ret == DLMS_OK) {
        printf("   HDLC connected\n");
    } else {
        printf("   HDLC connect result: %d\n", ret);
    }

    /* Create COSEM objects */
    printf("\n2. Creating COSEM objects...\n");
    dlms_cosem_object_list_t objects;
    dlms_cosem_list_init(&objects);

    dlms_data_object_t data_obj;
    dlms_obis_t ln_data = {{0,0,1,0,0,255}};
    dlms_data_create(&data_obj, &ln_data);
    dlms_value_t data_val;
    dlms_value_set_int32(&data_val, 12345);
    dlms_data_set_value(&data_obj, &data_val);
    dlms_cosem_list_add(&objects, &data_obj.base);

    dlms_register_object_t reg_obj;
    dlms_obis_t ln_reg = {{0,0,1,8,0,255}};
    dlms_register_create(&reg_obj, &ln_reg);
    dlms_value_t reg_val;
    dlms_value_set_uint32(&reg_val, 99999);
    dlms_register_set_value(&reg_obj, &reg_val);
    dlms_cosem_list_add(&objects, &reg_obj.base);

    dlms_clock_object_t clock_obj;
    dlms_obis_t ln_clock = {{0,0,1,0,0,255}};
    dlms_clock_create(&clock_obj, &ln_clock);
    dlms_cosem_list_add(&objects, &clock_obj.base);

    printf("   Created %d objects\n", objects.count);

    /* Read objects */
    printf("\n3. Reading object attributes...\n");
    for (uint16_t i = 0; i < objects.count; i++) {
        dlms_cosem_object_t *obj = objects.objects[i];
        char ln_str[32];
        dlms_obis_to_string(&obj->logical_name, ln_str, sizeof(ln_str));
        printf("   [%s] class=%d name=%s\n", ln_str, obj->class_id, obj->name);

        dlms_value_t val;
        if (obj->get_attr && obj->get_attr(obj, 2, &val) == DLMS_OK) {
            printf("     attr2: type=%s", dlms_data_type_name(val.type));
            switch (val.type) {
                case DLMS_DATA_DOUBLE_LONG:
                    printf(" value=%d\n", val.u.int32_val);
                    break;
                case DLMS_DATA_DOUBLE_LONG_UNSIGNED:
                    printf(" value=%u\n", val.u.uint32_val);
                    break;
                case DLMS_DATA_NULL:
                    printf(" (null)\n");
                    break;
                default:
                    printf("\n");
            }
        }
    }

    /* Security demo */
    printf("\n4. Security demo...\n");
    uint8_t key[16] = {0};
    memset(key, 0xAA, 16);
    uint8_t auth_key[16] = {0};
    memset(auth_key, 0xBB, 16);
    uint8_t system_title[8] = {0x4D,0x45,0x54,0x45,0x52,0x30,0x30,0x30};
    uint8_t iv[12];

    dlms_security_build_iv(system_title, 1, iv);
    printf("   IV: ");
    for (int i = 0; i < 12; i++) printf("%02X", iv[i]);
    printf("\n");

    dlms_security_control_t sc = {0};
    sc.security_suite = 5; /* SM4 */
    sc.authenticated = true;
    sc.encrypted = true;

    uint8_t plain[32] = "DLMS/COSEM test data!!!";
    uint8_t cipher[64];
    size_t w;
    ret = dlms_security_encrypt(&sc, system_title, 1, key, auth_key, plain, 32, cipher, sizeof(cipher), &w);
    printf("   Encrypt: %d bytes -> %zu bytes\n", 32, w);

    uint8_t dec[32];
    size_t dw;
    ret = dlms_security_decrypt(&sc, system_title, 1, key, auth_key, cipher, w, dec, sizeof(dec), &dw);
    printf("   Decrypt: %zu bytes, match=%s\n", dw, memcmp(dec, plain, 32) == 0 ? "yes" : "no");

    /* GMAC demo */
    uint8_t tag[12];
    ret = dlms_security_gmac(&sc, system_title, 1, key, auth_key,
                             (const uint8_t*)"authentication test", 20, tag);
    printf("   GMAC tag: ");
    for (int i = 0; i < 12; i++) printf("%02X", tag[i]);
    printf("\n");

    /* AXDR encode/decode demo */
    printf("\n5. AXDR encode/decode demo...\n");
    dlms_value_t av;
    dlms_value_set_int32(&av, 0x12345678);
    uint8_t axdr_buf[16];
    size_t aw;
    ret = dlms_axdr_encode(&av, axdr_buf, sizeof(axdr_buf), &aw);
    printf("   Encoded int32: ");
    for (size_t i = 0; i < aw; i++) printf("%02X", axdr_buf[i]);
    printf(" (%zu bytes)\n", aw);

    dlms_value_t dv;
    size_t dc;
    ret = dlms_axdr_decode(axdr_buf, aw, &dv, &dc);
    printf("   Decoded: %d (0x%08X)\n", dv.u.int32_val, dv.u.uint32_val);

    printf("\n=== Example complete ===\n");
    return 0;
}
