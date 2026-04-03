/* test_hdlc.c - HDLC tests */
#include "test_macros.h"
#include "dlms/dlms_core.h"
#include "dlms/dlms_hdlc.h"
#include <string.h>

void test_hdlc_crc(void) {
    uint16_t crc = dlms_crc16((const uint8_t*)"", 0);
    /* CRC of empty data - just verify it doesn't crash and is deterministic */
    ASSERT_EQ(crc, dlms_crc16((const uint8_t*)"", 0));

    uint8_t data1[] = {0xA0, 0x01, 0x03, 0x23, 0x01};
    uint16_t crc1 = dlms_crc16(data1, sizeof(data1));
    ASSERT_NEQ(crc1, 0);
    ASSERT_EQ(crc1, dlms_crc16(data1, sizeof(data1)));

    uint8_t data2[] = {0xA0, 0x01, 0x03, 0x23, 0x02};
    uint16_t crc3 = dlms_crc16(data2, sizeof(data2));
    ASSERT_NEQ(crc1, crc3);

    uint16_t partial = dlms_crc16(data1, 3);
    /* CRC update is incremental */
    uint16_t full = dlms_crc16_update(partial, data1 + 3, 2);
    /* Full should be same as computing on all 5 bytes */
    ASSERT_EQ(full, dlms_crc16(data1, 5));

    uint16_t single = dlms_crc16((const uint8_t*)"\x00", 1);
    ASSERT_NEQ(single, 0);
}

void test_hdlc_stuff(void) {
    uint8_t in1[] = {0x01, 0x02, 0x03};
    uint8_t out1[8];
    size_t n = dlms_hdlc_stuff(in1, 3, out1, sizeof(out1));
    ASSERT_EQ(n, 3u);
    ASSERT_MEM_EQ(in1, out1, 3);

    uint8_t in2[] = {0x7E, 0x01};
    uint8_t out2[8];
    n = dlms_hdlc_stuff(in2, 2, out2, sizeof(out2));
    ASSERT_EQ(n, 3u); /* 0x7E -> 2 bytes + 0x01 -> 1 byte */
    ASSERT_EQ(out2[0], 0x7D);
    ASSERT_EQ(out2[1], 0x5E);

    uint8_t in3[] = {0x7D};
    uint8_t out3[4];
    n = dlms_hdlc_stuff(in3, 1, out3, sizeof(out3));
    ASSERT_EQ(n, 2u);
    ASSERT_EQ(out3[0], 0x7D);
    ASSERT_EQ(out3[1], 0x5D);

    uint8_t in4[] = {0x00, 0x7E, 0x7D, 0xFF};
    uint8_t stuffed[16], unstuffed[16];
    n = dlms_hdlc_stuff(in4, 4, stuffed, sizeof(stuffed));
    size_t u = dlms_hdlc_unstuff(stuffed, n, unstuffed, sizeof(unstuffed));
    ASSERT_EQ(u, 4u);
    ASSERT_MEM_EQ(unstuffed, in4, 4);

    uint8_t small[1];
    n = dlms_hdlc_stuff(in2, 2, small, sizeof(small));
    ASSERT_EQ(n, 0u);
}

void test_hdlc_address(void) {
    dlms_hdlc_address_t addr = { .logical_address = 0x10, .has_physical = false };
    uint8_t dst[4];
    size_t w;
    ASSERT_OK(dlms_hdlc_address_encode(&addr, true, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 1u);
    ASSERT_EQ(dst[0], 0x21);

    addr.logical_address = 1;
    ASSERT_OK(dlms_hdlc_address_encode(&addr, false, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 2u);

    addr.logical_address = 0x100;
    addr.has_physical = true;
    addr.physical_address = 1;
    ASSERT_OK(dlms_hdlc_address_encode(&addr, false, dst, sizeof(dst), &w));
    ASSERT_EQ(w, 4u);
    ASSERT_TRUE(dst[3] & 0x01);

    dlms_hdlc_address_t decoded;
    size_t c;
    ASSERT_OK(dlms_hdlc_address_decode((const uint8_t*)"\x21", 1, true, &decoded, &c));
    ASSERT_EQ(decoded.logical_address, 0x10);
    ASSERT_EQ(c, 1u);

    ASSERT_OK(dlms_hdlc_address_decode((const uint8_t*)"\x00\x03", 2, false, &decoded, &c));
    ASSERT_FALSE(decoded.has_physical);

    ASSERT_EQ(dlms_hdlc_address_encode(NULL, true, dst, 4, &w), DLMS_ERROR_INVALID);
}

void test_hdlc_format(void) {
    dlms_hdlc_format_t fmt = { .length = 5, .segmented = false };
    uint8_t dst[2];
    ASSERT_OK(dlms_hdlc_format_encode(&fmt, dst));
    ASSERT_EQ(dst[0], 0xA0);
    ASSERT_EQ(dst[1], 0x05);

    fmt.segmented = true;
    dlms_hdlc_format_encode(&fmt, dst);
    ASSERT_NEQ(dst[0] & 0x08, 0);

    dlms_hdlc_format_t decoded;
    ASSERT_OK(dlms_hdlc_format_decode((const uint8_t*)"\xA0\x05", &decoded));
    ASSERT_EQ(decoded.length, 5);
    ASSERT_FALSE(decoded.segmented);

    ASSERT_OK(dlms_hdlc_format_decode((const uint8_t*)"\xA8\x05", &decoded));
    ASSERT_TRUE(decoded.segmented);

    ASSERT_EQ(dlms_hdlc_format_decode((const uint8_t*)"\xB0\x05", &decoded), DLMS_ERROR_PARSE);
}

void test_hdlc_control(void) {
    uint8_t dst;
    ASSERT_OK(dlms_hdlc_control_encode_snrm(&dst));
    ASSERT_EQ(dst, 0x93);
    ASSERT_OK(dlms_hdlc_control_encode_ua(&dst));
    ASSERT_EQ(dst, 0x73);
    ASSERT_OK(dlms_hdlc_control_encode_disc(&dst));
    ASSERT_EQ(dst, 0x53);

    ASSERT_OK(dlms_hdlc_control_encode_rr(3, &dst));
    /* Just verify it encodes without error and produces some non-zero value */
    ASSERT_NEQ(dst, 0);

    ASSERT_OK(dlms_hdlc_control_encode_info(1, 2, true, &dst));
    ASSERT_NEQ(dst, 0);

    ASSERT_OK(dlms_hdlc_control_encode_info(0, 0, false, &dst));
    ASSERT_EQ(dst, 0x00);

    ASSERT_OK(dlms_hdlc_control_encode_ui(true, &dst));
    ASSERT_EQ(dst, 0x13);

    dlms_hdlc_control_t ctrl;
    ASSERT_OK(dlms_hdlc_control_decode(0x42, DLMS_HDLC_FRAME_INFO, &ctrl));
    ASSERT_EQ(ctrl.send_seq, 1);
    ASSERT_EQ(ctrl.recv_seq, 2);

    ASSERT_OK(dlms_hdlc_control_decode(0x61, DLMS_HDLC_FRAME_RR, &ctrl));
    ASSERT_EQ(ctrl.recv_seq, 3);
}

void test_hdlc_params(void) {
    dlms_hdlc_param_list_t list;
    ASSERT_OK(dlms_hdlc_param_list_init(&list));
    ASSERT_EQ(list.count, 0);
    ASSERT_OK(dlms_hdlc_param_list_set(&list, DLMS_PARAM_WINDOW_SIZE, 3));
    ASSERT_EQ(list.count, 1);
    uint16_t val;
    ASSERT_OK(dlms_hdlc_param_list_get(&list, DLMS_PARAM_WINDOW_SIZE, &val));
    ASSERT_EQ(val, 3);
    ASSERT_OK(dlms_hdlc_param_list_set(&list, DLMS_PARAM_WINDOW_SIZE, 5));
    ASSERT_OK(dlms_hdlc_param_list_get(&list, DLMS_PARAM_WINDOW_SIZE, &val));
    ASSERT_EQ(val, 5);
    ASSERT_EQ(list.count, 1);
    ASSERT_EQ(dlms_hdlc_param_list_get(&list, DLMS_PARAM_MAX_INFO_TX, &val), DLMS_ERROR_NOT_SUPPORTED);

    uint8_t buf[32]; size_t w;
    ASSERT_OK(dlms_hdlc_param_list_encode(&list, buf, sizeof(buf), &w));
    ASSERT_EQ(w, 3u);
    ASSERT_EQ(buf[0], DLMS_PARAM_WINDOW_SIZE);
    ASSERT_EQ(buf[2], 5);

    dlms_hdlc_param_list_set(&list, DLMS_PARAM_MAX_INFO_TX, 512);
    dlms_hdlc_param_list_set(&list, DLMS_PARAM_MAX_INFO_RX, 1024);
    ASSERT_EQ(list.count, 3);

    dlms_hdlc_param_list_t list2;
    ASSERT_OK(dlms_hdlc_param_list_decode(buf, w, &list2));
    ASSERT_OK(dlms_hdlc_param_list_get(&list2, DLMS_PARAM_WINDOW_SIZE, &val));
    ASSERT_EQ(val, 5);
}

void test_hdlc_frame(void) {
    dlms_hdlc_address_t dest = { .logical_address = 1, .has_physical = false };
    dlms_hdlc_address_t src = { .logical_address = 16, .has_physical = false };
    dlms_hdlc_param_list_t params;
    dlms_hdlc_param_list_init(&params);
    dlms_hdlc_param_list_set(&params, DLMS_PARAM_WINDOW_SIZE, 1);

    uint8_t frame_buf[128];
    int len = dlms_hdlc_build_snrm(frame_buf, sizeof(frame_buf), &dest, &src, &params);
    ASSERT_GT(len, 0);
    dlms_hdlc_frame_t frame;
    ASSERT_OK(dlms_hdlc_frame_decode(frame_buf, (size_t)len, &frame));
    ASSERT_EQ(frame.type, DLMS_HDLC_FRAME_SNRM);

    len = dlms_hdlc_build_ua(frame_buf, sizeof(frame_buf), &src, &dest, &params);
    ASSERT_GT(len, 0);
    ASSERT_OK(dlms_hdlc_frame_decode(frame_buf, (size_t)len, &frame));
    ASSERT_EQ(frame.type, DLMS_HDLC_FRAME_UA);

    len = dlms_hdlc_build_rr(frame_buf, sizeof(frame_buf), &dest, &src, 0);
    ASSERT_GT(len, 0);
    ASSERT_OK(dlms_hdlc_frame_decode(frame_buf, (size_t)len, &frame));
    ASSERT_EQ(frame.type, DLMS_HDLC_FRAME_RR);

    uint8_t info[] = {0xC0, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0xFF, 0x02};
    len = dlms_hdlc_build_info(frame_buf, sizeof(frame_buf), &dest, &src, 0, 0, true, info, sizeof(info));
    ASSERT_GT(len, 0);
    ASSERT_OK(dlms_hdlc_frame_decode(frame_buf, (size_t)len, &frame));
    ASSERT_EQ(frame.type, DLMS_HDLC_FRAME_INFO);

    len = dlms_hdlc_build_disc(frame_buf, sizeof(frame_buf), &dest, &src);
    ASSERT_GT(len, 0);
    ASSERT_OK(dlms_hdlc_frame_decode(frame_buf, (size_t)len, &frame));
    ASSERT_EQ(frame.type, DLMS_HDLC_FRAME_DISC);

    /* Invalid frame */
    int r = dlms_hdlc_frame_decode((const uint8_t*)"\x7E\x00\x7E", 3, &frame);
    ASSERT_NEQ(r, DLMS_OK);
}

void test_hdlc_parser(void) {
    static uint8_t parser_buf[256];
    dlms_hdlc_parser_t *parser = dlms_hdlc_parser_create(parser_buf, sizeof(parser_buf));
    dlms_hdlc_address_t dest = { .logical_address = 1, .has_physical = false };
    dlms_hdlc_address_t src = { .logical_address = 16, .has_physical = false };
    dlms_hdlc_param_list_t params;
    dlms_hdlc_param_list_init(&params);
    uint8_t frame[128];
    int len = dlms_hdlc_build_snrm(frame, sizeof(frame), &dest, &src, &params);
    ASSERT_GT(len, 0);
    ASSERT_OK(dlms_hdlc_parse(parser, frame, (size_t)len, NULL, NULL));
    dlms_hdlc_parser_reset(parser);
    for (int i = 0; i < len; i++) dlms_hdlc_parse(parser, frame + i, 1, NULL, NULL);
}

void test_hdlc_build(void) {
    dlms_hdlc_address_t dest = { .logical_address = 0x01, .has_physical = false };
    dlms_hdlc_address_t src = { .logical_address = 0x10, .has_physical = false };
    uint8_t buf[64];
    int len = dlms_hdlc_build_snrm(buf, sizeof(buf), &dest, &src, NULL);
    ASSERT_GT(len, 0);
    ASSERT_EQ(buf[0], 0x7E);
    ASSERT_EQ(buf[len - 1], 0x7E);
    len = dlms_hdlc_build_ua(buf, sizeof(buf), &dest, &src, NULL);
    ASSERT_GT(len, 0);
    len = dlms_hdlc_build_rr(buf, sizeof(buf), &dest, &src, 0);
    ASSERT_GT(len, 0);
    len = dlms_hdlc_build_disc(buf, sizeof(buf), &dest, &src);
    ASSERT_GT(len, 0);
    len = dlms_hdlc_build_info(buf, sizeof(buf), &dest, &src, 0, 0, true, NULL, 0);
    ASSERT_GT(len, 0);
}
