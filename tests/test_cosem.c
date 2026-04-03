/* test_cosem.c - COSEM tests */
#include "test_macros.h"
#include "dlms/dlms_core.h"
#include "dlms/dlms_cosem.h"
#include <string.h>

void test_cosem_data(void) {
    dlms_data_object_t obj;
    dlms_obis_t ln = {{0,0,1,0,0,255}};
    ASSERT_OK(dlms_data_create(&obj, &ln));
    ASSERT_EQ(obj.base.class_id, DLMS_IC_DATA);
    ASSERT_TRUE(dlms_obis_equals(&obj.base.logical_name, &ln));

    /* Get logical name (attr 1) */
    dlms_value_t val;
    ASSERT_OK(obj.base.get_attr(&obj.base, 1, &val));
    ASSERT_EQ(val.type, DLMS_DATA_OCTET_STRING);
    ASSERT_EQ(val.octet_string.len, 6);

    /* Get value (attr 2) - default null */
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.type, DLMS_DATA_NULL);

    /* Set value */
    dlms_value_set_int32(&val, 42);
    ASSERT_OK(dlms_data_set_value(&obj, &val));
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.type, DLMS_DATA_DOUBLE_LONG);
    ASSERT_EQ(val.int32_val, 42);

    /* Set via base interface */
    dlms_value_t new_val;
    dlms_value_set_string(&new_val, "test", 4);
    ASSERT_OK(obj.base.set_attr(&obj.base, 2, &new_val));
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.type, DLMS_DATA_VISIBLE_STRING);

    /* Invalid attribute */
    ASSERT_EQ(obj.base.get_attr(&obj.base, 99, &val), DLMS_ERROR_NOT_SUPPORTED);
    ASSERT_EQ(obj.base.set_attr(&obj.base, 99, &val), DLMS_ERROR_NOT_SUPPORTED);

    /* Error cases */
    ASSERT_EQ(dlms_data_create(NULL, &ln), DLMS_ERROR_INVALID);
    ASSERT_EQ(dlms_data_create(&obj, NULL), DLMS_ERROR_INVALID);
}

void test_cosem_register(void) {
    dlms_register_object_t obj;
    dlms_obis_t ln = {{0,0,1,8,0,255}};
    ASSERT_OK(dlms_register_create(&obj, &ln));
    ASSERT_EQ(obj.base.class_id, DLMS_IC_REGISTER);

    /* Get value (attr 2) */
    dlms_value_t val;
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.type, DLMS_DATA_DOUBLE_LONG);
    ASSERT_EQ(val.int32_val, 0);

    /* Set value */
    dlms_value_set_uint32(&val, 12345);
    ASSERT_OK(dlms_register_set_value(&obj, &val));
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.uint32_val, 12345u);

    /* Scaler/unit (attr 3) */
    ASSERT_OK(obj.base.get_attr(&obj.base, 3, &val));

    /* Set scaler/unit */
    dlms_value_t su;
    dlms_value_set_int8(&su, -1);
    ASSERT_OK(obj.base.set_attr(&obj.base, 3, &su));

    /* Error cases */
    ASSERT_EQ(dlms_register_create(NULL, &ln), DLMS_ERROR_INVALID);
}

void test_cosem_clock(void) {
    dlms_clock_object_t obj;
    dlms_obis_t ln = {{0,0,1,0,0,255}};
    ASSERT_OK(dlms_clock_create(&obj, &ln));
    ASSERT_EQ(obj.base.class_id, DLMS_IC_CLOCK);

    /* Get datetime (attr 2) */
    dlms_value_t val;
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));

    /* Set datetime */
    val.type = DLMS_DATA_DATETIME;
    memset(&val.datetime, 0, sizeof(val.datetime));
    val.datetime.year = 2024;
    val.datetime.month = 6;
    val.datetime.day = 15;
    val.datetime.hour = 12;
    val.datetime.minute = 30;
    val.datetime.second = 0;
    ASSERT_OK(obj.base.set_attr(&obj.base, 2, &val));
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.datetime.year, 2024);

    /* Timezone (attr 3) */
    ASSERT_OK(obj.base.get_attr(&obj.base, 3, &val));
    ASSERT_EQ(val.type, DLMS_DATA_OCTET_STRING);
    ASSERT_EQ(val.octet_string.len, 3);

    /* Set timezone */
    uint8_t tz_buf[3] = {0x00, 0x3C, 0x00}; /* +60 min, no DST */
    dlms_value_set_octet(&val, tz_buf, 3);
    ASSERT_OK(obj.base.set_attr(&obj.base, 3, &val));
    ASSERT_EQ(obj.timezone, 60);
}

void test_cosem_security_setup(void) {
    dlms_security_setup_object_t obj;
    dlms_obis_t ln = {{0,0,43,0,0,255}};
    ASSERT_OK(dlms_security_setup_create(&obj, &ln));
    ASSERT_EQ(obj.base.class_id, DLMS_IC_SECURITY_SETUP);

    /* Get security suite (attr 2) */
    dlms_value_t val;
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.type, DLMS_DATA_UNSIGNED_INTEGER);

    /* Error cases */
    ASSERT_EQ(dlms_security_setup_create(NULL, &ln), DLMS_ERROR_INVALID);
}

void test_cosem_profile_generic(void) {
    dlms_profile_generic_object_t obj;
    dlms_obis_t ln = {{1,0,99,1,0,255}};
    ASSERT_OK(dlms_profile_generic_create(&obj, &ln));
    ASSERT_EQ(obj.base.class_id, DLMS_IC_PROFILE_GENERIC);

    /* Get logical name */
    dlms_value_t val;
    ASSERT_OK(obj.base.get_attr(&obj.base, 1, &val));
    ASSERT_EQ(val.type, DLMS_DATA_OCTET_STRING);

    /* Buffer (attr 2) - may be null or array */
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));

    /* Error cases */
    ASSERT_EQ(dlms_profile_generic_create(NULL, &ln), DLMS_ERROR_INVALID);
}

void test_cosem_list(void) {
    dlms_cosem_object_list_t list;
    ASSERT_OK(dlms_cosem_list_init(&list));
    ASSERT_EQ(list.count, 0);

    dlms_data_object_t data1, data2;
    dlms_obis_t ln1 = {{0,0,1,0,0,255}};
    dlms_obis_t ln2 = {{0,0,1,1,0,255}};
    dlms_data_create(&data1, &ln1);
    dlms_data_create(&data2, &ln2);

    ASSERT_OK(dlms_cosem_list_add(&list, &data1.base));
    ASSERT_EQ(list.count, 1);
    ASSERT_OK(dlms_cosem_list_add(&list, &data2.base));
    ASSERT_EQ(list.count, 2);

    /* Find by LN */
    dlms_cosem_object_t *found = dlms_cosem_list_find_by_ln(&list, &ln1);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->class_id, DLMS_IC_DATA);

    found = dlms_cosem_list_find_by_ln(&list, &ln2);
    ASSERT_TRUE(found != NULL);

    /* Find by class + LN */
    found = dlms_cosem_list_find(&list, DLMS_IC_DATA, &ln1);
    ASSERT_TRUE(found != NULL);

    /* Not found */
    dlms_obis_t ln3 = {{9,9,9,9,9,9}};
    found = dlms_cosem_list_find_by_ln(&list, &ln3);
    ASSERT_TRUE(found == NULL);

    /* Error cases */
    ASSERT_EQ(dlms_cosem_list_add(NULL, &data1.base), DLMS_ERROR_INVALID);
    ASSERT_TRUE(dlms_cosem_list_find(NULL, DLMS_IC_DATA, &ln1) == NULL);
}

void test_cosem_association(void) {
    dlms_association_ln_object_t obj;
    dlms_obis_t ln = {{0,0,40,0,0,255}};
    ASSERT_OK(dlms_association_ln_create(&obj, &ln));
    ASSERT_EQ(obj.base.class_id, DLMS_IC_ASSOCIATION_LN);

    /* Get auth mechanism (attr 2) */
    dlms_value_t val;
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.type, DLMS_DATA_UNSIGNED_INTEGER);

    /* Set auth */
    obj.auth_mechanism = DLMS_AUTH_HLS_GMAC;
    ASSERT_OK(obj.base.get_attr(&obj.base, 2, &val));
    ASSERT_EQ(val.uint8_val, DLMS_AUTH_HLS_GMAC);
}
