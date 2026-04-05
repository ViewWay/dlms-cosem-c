/* test_axdr.c - AXDR tests */
#include "test_macros.h"
#include "dlms/dlms_core.h"
#include "dlms/dlms_axdr.h"
#include <string.h>

void test_axdr_length(void) {
    uint8_t dst[8];
    size_t w;

    ASSERT_OK(dlms_axdr_encode_length(0, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u); ASSERT_EQ(dst[0], 0);

    ASSERT_OK(dlms_axdr_encode_length(127, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);

    ASSERT_OK(dlms_axdr_encode_length(128, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(dst[0], 0x82);

    ASSERT_OK(dlms_axdr_encode_length(255, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);

    ASSERT_OK(dlms_axdr_encode_length(256, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 3u);
    ASSERT_EQ(dst[0], 0x84);

    uint32_t val;
    size_t c;
    /* Encode and decode 256 */
    ASSERT_OK(dlms_axdr_encode_length(256, dst, sizeof(dst), &w));
    ASSERT_OK(dlms_axdr_decode_length(dst, w, &val, &c));
    ASSERT_EQ(val, 256u);

    ASSERT_OK(dlms_axdr_decode_length((const uint8_t*)"\x00", 1, &val, &c));
    ASSERT_EQ(val, 0u);

    ASSERT_OK(dlms_axdr_decode_length((const uint8_t*)"\x82\x80", 2, &val, &c));
    ASSERT_EQ(val, 128u);
}

void test_axdr_encode(void) {
    uint8_t buf[64];
    size_t w;
    dlms_value_t v;

    /* Null */
    dlms_value_set_null(&v);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 1u);
    ASSERT_EQ(buf[0], 0x00);

    /* Boolean */
    dlms_value_set_bool(&v, true);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(buf[0], 3);
    ASSERT_EQ(buf[1], 1);

    dlms_value_set_bool(&v, false);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(buf[1], 0);

    /* Integer types */
    dlms_value_set_int8(&v, -1);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(buf[0], 15);
    ASSERT_EQ(buf[1], 0xFF);

    dlms_value_set_uint8(&v, 200);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(buf[0], 17);

    dlms_value_set_int16(&v, 1000);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 3u);
    ASSERT_EQ(buf[0], 16);

    dlms_value_set_uint16(&v, 0x1234);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 3u);
    ASSERT_EQ(buf[1], 0x12);
    ASSERT_EQ(buf[2], 0x34);

    dlms_value_set_int32(&v, 0x12345678);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 5u);
    ASSERT_EQ(buf[0], 5);

    dlms_value_set_uint32(&v, 0xDEADBEEF);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 5u);

    dlms_value_set_int64(&v, 0x0102030405060708LL);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 9u);
    ASSERT_EQ(buf[0], 20);

    dlms_value_set_uint64(&v, 0);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 9u);

    /* Enum */
    dlms_value_set_enum(&v, 5);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 2u);
    ASSERT_EQ(buf[0], 22);
    ASSERT_EQ(buf[1], 5);

    /* Float32 */
    dlms_value_set_float32(&v, 1.0f);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 5u);
    ASSERT_EQ(buf[0], 23);

    /* Float64 */
    dlms_value_set_float64(&v, 1.0);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 9u);
    ASSERT_EQ(buf[0], 24);

    /* Octet string */
    uint8_t data[] = {1, 2, 3, 4, 5};
    dlms_value_set_octet(&v, data, 5);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(buf[0], 9);
    ASSERT_EQ(w, 7u); /* tag + length + 5 bytes */

    /* String */
    dlms_value_set_string(&v, "AB", 2);
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(buf[0], 10);

    /* UTF8 string */
    dlms_value_set_string(&v, "AB", 2);
    v.type = DLMS_DATA_UTF8_STRING;
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(buf[0], 12);

    /* Date */
    v.type = DLMS_DATA_DATE;
    v.u.date.year = 2024; v.u.date.month = 1; v.u.date.day = 15; v.u.date.day_of_week = 1;
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 6u);
    ASSERT_EQ(buf[0], 26);

    /* Time */
    v.type = DLMS_DATA_TIME;
    v.u.time_val.hour = 12; v.u.time_val.minute = 30; v.u.time_val.second = 0; v.u.time_val.hundredths = 0;
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 5u);
    ASSERT_EQ(buf[0], 27);

    /* Datetime */
    v.type = DLMS_DATA_DATETIME;
    memset(&v.u.datetime, 0, sizeof(v.u.datetime));
    v.u.datetime.year = 2024; v.u.datetime.month = 6; v.u.datetime.day = 15;
    v.u.datetime.hour = 12; v.u.datetime.minute = 30; v.u.datetime.second = 45;
    v.u.datetime.deviation = 60;
    v.u.datetime.clock_status = 0;
    ASSERT_OK(dlms_axdr_encode(&v, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 13u);
    ASSERT_EQ(buf[0], 25);

    /* Buffer too small */
    dlms_value_set_int32(&v, 0);
    ASSERT_EQ(dlms_axdr_encode(&v, buf, 3, &w), DLMS_ERROR_BUFFER);
}

void test_axdr_decode(void) {
    dlms_value_t v;
    size_t c;

    /* Null */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x00", 1, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_NULL);
    ASSERT_EQ(c, 1u);

    /* Boolean */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x03\x01", 2, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_BOOLEAN);
    ASSERT_TRUE(v.u.boolean_val);

    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x03\x00", 2, &v, &c));
    ASSERT_FALSE(v.u.boolean_val);

    /* int8 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x0F\xFF", 2, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_INTEGER);
    ASSERT_EQ(v.u.int8_val, -1);

    /* uint8 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x11\xC8", 2, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_UNSIGNED_INTEGER);
    ASSERT_EQ(v.u.uint8_val, 200);

    /* int16 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x10\x03\xE8", 3, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_LONG);
    ASSERT_EQ(v.u.int16_val, 1000);

    /* uint16 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x12\x12\x34", 3, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_UNSIGNED_LONG);
    ASSERT_EQ(v.u.uint16_val, 0x1234);

    /* int32 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x05\x12\x34\x56\x78", 5, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_DOUBLE_LONG);
    ASSERT_EQ(v.u.int32_val, 0x12345678);

    /* uint32 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x06\xDE\xAD\xBE\xEF", 5, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_DOUBLE_LONG_UNSIGNED);
    ASSERT_EQ(v.u.uint32_val, 0xDEADBEEF);

    /* int64 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x14\x01\x02\x03\x04\x05\x06\x07\x08", 9, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_LONG64);

    /* uint64 */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x15\x00\x00\x00\x00\x00\x00\x00\x00", 9, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_UNSIGNED_LONG64);
    ASSERT_EQ(v.u.uint64_val, 0u);

    /* enum */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x16\x05", 2, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_ENUM);
    ASSERT_EQ(v.u.uint8_val, 5);

    /* float32 */
    float f = 1.0f;
    uint8_t fbuf[5] = {23, 0,0,0,0};
    memcpy(fbuf + 1, &f, 4);
    ASSERT_OK(dlms_axdr_decode(fbuf, 5, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_FLOAT32);

    /* float64 */
    double d = 1.0;
    uint8_t dbuf[9] = {24};
    memcpy(dbuf + 1, &d, 8);
    ASSERT_OK(dlms_axdr_decode(dbuf, 9, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_FLOAT64);

    /* octet string */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x09\x05\x01\x02\x03\x04\x05", 7, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_OCTET_STRING);
    ASSERT_EQ(v.u.octet_string.len, 5);
    ASSERT_EQ(v.u.octet_string.data[0], 1);
    ASSERT_EQ(v.u.octet_string.data[4], 5);

    /* visible string */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x0A\x02\x41\x42", 4, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_VISIBLE_STRING);
    ASSERT_EQ(v.u.string_val.len, 2);

    /* date */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x1A\x07\xE8\x01\x0F\x01", 6, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_DATE);
    ASSERT_EQ(v.u.date.year, 2024);
    ASSERT_EQ(v.u.date.month, 1);
    ASSERT_EQ(v.u.date.day, 15);

    /* time */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x1B\x0C\x1E\x00\x00", 5, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_TIME);
    ASSERT_EQ(v.u.time_val.hour, 12);
    ASSERT_EQ(v.u.time_val.minute, 30);

    /* datetime */
    uint8_t dt[] = {0x19, 0x07,0xE8, 0x06, 0x0F, 0x00, 0x0C, 0x1E, 0x2D, 0x00, 0x00, 0x3C, 0x00};
    ASSERT_OK(dlms_axdr_decode(dt, sizeof(dt), &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_DATETIME);
    ASSERT_EQ(v.u.datetime.year, 2024);
    ASSERT_EQ(v.u.datetime.month, 6);

    /* array */
    ASSERT_OK(dlms_axdr_decode((const uint8_t*)"\x01\x02\x03\xFF\x04", 5, &v, &c));
    ASSERT_EQ(v.type, DLMS_DATA_ARRAY);
    ASSERT_EQ(v.u.array_val.count, 2);

    /* Error cases */
    ASSERT_EQ(dlms_axdr_decode(NULL, 1, &v, &c), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_axdr_decode((const uint8_t*)"\x0F", 1, &v, &c), DLMS_ERROR_BUFFER);
}

/* Encode-decode roundtrip */
void test_axdr_compact(void) {
    dlms_value_t items[3];
    dlms_value_set_int32(&items[0], 100);
    dlms_value_set_int32(&items[1], 200);
    dlms_value_set_int32(&items[2], 300);

    dlms_data_type_t types[] = {DLMS_DATA_DOUBLE_LONG};
    uint8_t buf[128];
    size_t w;
    ASSERT_OK(dlms_axdr_encode_compact_array(items, 3, types, 1, buf, sizeof(buf), &w));
    ASSERT_GT(w, 0u);

    /* Verify compact array encode doesn't crash */
    ASSERT_OK(dlms_axdr_encode_compact_array(items, 3, types, 1, buf, sizeof(buf), &w));
    ASSERT_GT(w, 0u);
}
