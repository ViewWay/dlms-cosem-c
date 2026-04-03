/* test_core.c - Core tests */
#include "test_macros.h"
#include "dlms/dlms_core.h"

void test_core_obis(void) {
    dlms_obis_t obis;
    uint8_t buf[] = {0,1,1,0,0,255};
    ASSERT_OK(dlms_obis_parse(buf, 6, &obis));
    ASSERT_EQ(obis.bytes[0], 0);
    ASSERT_EQ(obis.bytes[1], 1);
    ASSERT_EQ(obis.bytes[2], 1);
    ASSERT_EQ(obis.bytes[5], 255);

    char str[32];
    ASSERT_OK(dlms_obis_to_string(&obis, str, sizeof(str)));
    /* Verify string content */
    ASSERT_EQ(str[0], '0');

    dlms_obis_t obis2;
    ASSERT_OK(dlms_obis_from_string("1.1.1.0.0.255", &obis2));
    ASSERT_EQ(obis2.bytes[0], 1);
    ASSERT_EQ(obis2.bytes[5], 255);

    ASSERT_TRUE(dlms_obis_equals(&obis2, &obis2));
    ASSERT_FALSE(dlms_obis_equals(&obis, &obis2));

    dlms_obis_t wild;
    memset(&wild, 0xFF, sizeof(wild));
    ASSERT_TRUE(dlms_obis_is_wildcard(&wild));
    ASSERT_FALSE(dlms_obis_is_wildcard(&obis));

    ASSERT_EQ(dlms_obis_parse(NULL, 6, &obis), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_obis_parse(buf, 5, &obis), DLMS_ERROR_INVALID);
}

void test_core_buffer(void) {
    uint8_t data[64];
    dlms_buffer_t buf;
    dlms_buffer_init(&buf, data, sizeof(data));
    ASSERT_EQ(buf.len, 0u);
    ASSERT_EQ(buf.cap, 64u);

    ASSERT_OK(dlms_buffer_append(&buf, (const uint8_t*)"AB", 2));
    ASSERT_EQ(buf.len, 2u);
    ASSERT_OK(dlms_buffer_append_byte(&buf, 'C'));
    ASSERT_EQ(buf.len, 3u);
    ASSERT_EQ(buf.data[2], 'C');

    ASSERT_EQ(dlms_buffer_remaining(&buf), 61);

    dlms_buffer_clear(&buf);
    ASSERT_EQ(buf.len, 0u);

    ASSERT_EQ(dlms_buffer_append(NULL, data, 1), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_buffer_append(&buf, NULL, 1), DLMS_ERROR_INVALID);

    uint8_t small[1];
    dlms_buffer_t sb;
    dlms_buffer_init(&sb, small, 1);
    ASSERT_EQ(dlms_buffer_append(&sb, (const uint8_t*)"AB", 2), DLMS_ERROR_BUFFER);
}

void test_core_varint(void) {
    uint8_t dst[8];
    size_t w;

    ASSERT_OK(dlms_varint_encode(0, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);
    ASSERT_EQ(dst[0], 0);

    ASSERT_OK(dlms_varint_encode(127, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);
    ASSERT_EQ(dst[0], 127);

    ASSERT_OK(dlms_varint_encode(128, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(dst[0], 0x81);
    ASSERT_EQ(dst[1], 128);

    ASSERT_OK(dlms_varint_encode(255, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(dst[1], 0xFF);

    ASSERT_OK(dlms_varint_encode(65535, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 3u);

    uint32_t val;
    size_t c;
    ASSERT_OK(dlms_varint_decode(dst, w, &val, &c));
    ASSERT_EQ(val, 65535u);
    ASSERT_EQ(c, 3u);

    ASSERT_OK(dlms_varint_decode((const uint8_t*)"\x42", 1, &val, &c));
    ASSERT_EQ(val, 66u);

    ASSERT_EQ(dlms_varint_decode(NULL, 1, &val, &c), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_varint_decode((const uint8_t*)"\x80", 1, &val, &c), DLMS_ERROR_PARSE);
}

void test_core_value(void) {
    dlms_value_t v;
    dlms_value_set_null(&v);
    ASSERT_EQ(v.type, DLMS_DATA_NULL);

    dlms_value_set_bool(&v, true);
    ASSERT_EQ(v.type, DLMS_DATA_BOOLEAN);
    ASSERT_TRUE(v.boolean_val);

    dlms_value_set_int8(&v, -42);
    ASSERT_EQ(v.int8_val, -42);

    dlms_value_set_uint8(&v, 200);
    ASSERT_EQ(v.uint8_val, 200);

    dlms_value_set_int16(&v, -1000);
    ASSERT_EQ(v.int16_val, -1000);

    dlms_value_set_uint16(&v, 60000);
    ASSERT_EQ(v.uint16_val, 60000);

    dlms_value_set_int32(&v, -100000);
    ASSERT_EQ(v.int32_val, -100000);

    dlms_value_set_uint32(&v, 4000000000U);
    ASSERT_EQ(v.uint32_val, 4000000000U);

    dlms_value_set_int64(&v, -1);
    ASSERT_EQ(v.int64_val, -1);

    dlms_value_set_uint64(&v, 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(v.uint64_val, 0xFFFFFFFFFFFFFFFFULL);

    dlms_value_set_enum(&v, 5);
    ASSERT_EQ(v.uint8_val, 5);

    uint8_t data[] = {1,2,3};
    dlms_value_set_octet(&v, data, 3);
    ASSERT_EQ(v.octet_string.len, 3);
    ASSERT_MEM_EQ(v.octet_string.data, data, 3);

    dlms_value_set_string(&v, "hello", 5);
    ASSERT_EQ(v.string_val.len, 5);

    dlms_value_set_float32(&v, 3.14f);
    ASSERT_TRUE(v.float32_val > 3.0f && v.float32_val < 3.2f);

    dlms_value_set_float64(&v, 2.71828);
    ASSERT_TRUE(v.float64_val > 2.7 && v.float64_val < 2.8);
}

void test_core_data_type(void) {
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_NULL), 0);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_BOOLEAN), 1);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_INTEGER), 1);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_LONG), 2);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_DOUBLE_LONG), 4);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_UNSIGNED_LONG64), 8);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_FLOAT32), 4);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_FLOAT64), 8);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_DATETIME), 12);
    ASSERT_EQ(dlms_data_type_fixed_length(DLMS_DATA_OCTET_STRING), 0xFFFF);

    const char *n = dlms_data_type_name(DLMS_DATA_BOOLEAN);
    ASSERT_EQ(n[0], 'b');
    n = dlms_data_type_name(DLMS_DATA_ARRAY);
    ASSERT_EQ(n[0], 'a');
}

void test_core_bits(void) {
    ASSERT_EQ(dlms_reverse_bits8(0x00), 0x00);
    ASSERT_EQ(dlms_reverse_bits8(0xFF), 0xFF);
    ASSERT_EQ(dlms_reverse_bits8(0x01), 0x80);
    ASSERT_EQ(dlms_reverse_bits8(0x80), 0x01);
    ASSERT_EQ(dlms_reverse_bits8(0xAA), 0x55);

    ASSERT_EQ(dlms_reverse_bits16(0x0000), 0x0000);
    ASSERT_EQ(dlms_reverse_bits16(0xFFFF), 0xFFFF);
    ASSERT_EQ(dlms_reverse_bits16(0x0001), 0x8000);
    ASSERT_EQ(dlms_reverse_bits16(0x8000), 0x0001);
}
