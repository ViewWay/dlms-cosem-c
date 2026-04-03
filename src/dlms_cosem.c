/**
 * @file dlms_cosem.c
 * @brief COSEM Interface Classes implementation
 */
#include "dlms/dlms_cosem.h"
#include <string.h>
#include <stdio.h>

/* ===== IC: Data ===== */

static int data_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_data_object_t *d = (dlms_data_object_t *)obj;
    if (attr_index == 2) { *value = d->value; return DLMS_OK; }
    if (attr_index == 1) { /* logical name */
        uint8_t ln[6];
        memcpy(ln, d->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

static int data_set_attr(dlms_cosem_object_t *obj, uint8_t attr_index, const dlms_value_t *value) {
    dlms_data_object_t *d = (dlms_data_object_t *)obj;
    if (attr_index == 2) { d->value = *value; return DLMS_OK; }
    return DLMS_ERROR_NOT_SUPPORTED;
}

static int data_action(dlms_cosem_object_t *obj, uint8_t method_index, dlms_value_t *params, dlms_value_t *result) {
    (void)obj; (void)method_index; (void)params; (void)result;
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_data_create(dlms_data_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_DATA;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Data";
    obj->base.get_attr = data_get_attr;
    obj->base.set_attr = data_set_attr;
    obj->base.action = data_action;
    obj->base.attr_count = 2;
    obj->base.method_count = 0;
    dlms_value_set_null(&obj->value);
    return DLMS_OK;
}

int dlms_data_set_value(dlms_data_object_t *obj, const dlms_value_t *value) {
    if (!obj || !value) return DLMS_ERROR_INVALID;
    obj->value = *value;
    return DLMS_OK;
}

/* ===== IC: Register ===== */

static int register_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_register_object_t *r = (dlms_register_object_t *)obj;
    if (attr_index == 2) { *value = r->value; return DLMS_OK; }
    if (attr_index == 3) { *value = r->scaler_unit; return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, r->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

static int register_set_attr(dlms_cosem_object_t *obj, uint8_t attr_index, const dlms_value_t *value) {
    dlms_register_object_t *r = (dlms_register_object_t *)obj;
    if (attr_index == 2) { r->value = *value; return DLMS_OK; }
    if (attr_index == 3) { r->scaler_unit = *value; return DLMS_OK; }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_register_create(dlms_register_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_REGISTER;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Register";
    obj->base.get_attr = register_get_attr;
    obj->base.set_attr = register_set_attr;
    obj->base.action = data_action;
    obj->base.attr_count = 3;
    obj->base.method_count = 0;
    dlms_value_set_int32(&obj->value, 0);
    return DLMS_OK;
}

int dlms_register_set_value(dlms_register_object_t *obj, const dlms_value_t *value) {
    if (!obj || !value) return DLMS_ERROR_INVALID;
    obj->value = *value;
    return DLMS_OK;
}

/* ===== IC: Clock ===== */

static int clock_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_clock_object_t *c = (dlms_clock_object_t *)obj;
    if (attr_index == 2) { *value = c->datetime; return DLMS_OK; }
    if (attr_index == 3) {
        /* TimeZone + dst */
        int16_t tz = c->timezone;
        uint8_t buf[3];
        buf[0] = (uint8_t)(tz >> 8);
        buf[1] = (uint8_t)(tz & 0xFF);
        buf[2] = c->dst_status;
        dlms_value_set_octet(value, buf, 3);
        return DLMS_OK;
    }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, c->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

static int clock_set_attr(dlms_cosem_object_t *obj, uint8_t attr_index, const dlms_value_t *value) {
    dlms_clock_object_t *c = (dlms_clock_object_t *)obj;
    if (attr_index == 2) { c->datetime = *value; return DLMS_OK; }
    if (attr_index == 3) {
        if (value->type == DLMS_DATA_OCTET_STRING && value->octet_string.len >= 3) {
            c->timezone = (int16_t)((value->octet_string.data[0] << 8) | value->octet_string.data[1]);
            c->dst_status = value->octet_string.data[2];
            return DLMS_OK;
        }
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

static int clock_action(dlms_cosem_object_t *obj, uint8_t method_index, dlms_value_t *params, dlms_value_t *result) {
    (void)obj; (void)method_index; (void)params; (void)result;
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_clock_create(dlms_clock_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_CLOCK;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Clock";
    obj->base.get_attr = clock_get_attr;
    obj->base.set_attr = clock_set_attr;
    obj->base.action = clock_action;
    obj->base.attr_count = 8;
    obj->base.method_count = 6;
    dlms_value_set_null(&obj->datetime);
    obj->timezone = 0;
    obj->dst_status = 0;
    return DLMS_OK;
}

/* ===== IC: SecuritySetup ===== */

static int security_setup_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_security_setup_object_t *s = (dlms_security_setup_object_t *)obj;
    if (attr_index == 2) { dlms_value_set_uint8(value, s->security_suite); return DLMS_OK; }
    if (attr_index == 3) { dlms_value_set_octet(value, s->enc_key, s->enc_key_len); return DLMS_OK; }
    if (attr_index == 4) { dlms_value_set_octet(value, s->auth_key, s->auth_key_len); return DLMS_OK; }
    if (attr_index == 7) { dlms_value_set_octet(value, s->system_title, s->system_title_len); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, s->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_security_setup_create(dlms_security_setup_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_SECURITY_SETUP;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "SecuritySetup";
    obj->base.get_attr = security_setup_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 7;
    obj->base.method_count = 0;
    return DLMS_OK;
}

/* ===== IC: Association LN ===== */

static int assoc_ln_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_association_ln_object_t *a = (dlms_association_ln_object_t *)obj;
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, a->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    if (attr_index == 2) {
        dlms_value_set_uint8(value, a->auth_mechanism);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_association_ln_create(dlms_association_ln_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_ASSOCIATION_LN;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "AssociationLN";
    obj->base.get_attr = assoc_ln_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 8;
    obj->base.method_count = 6;
    return DLMS_OK;
}

/* ===== IC: Profile Generic ===== */

static int profile_generic_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_profile_generic_object_t *p = (dlms_profile_generic_object_t *)obj;
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, p->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    if (attr_index == 2) { /* buffer */
        if (p->buffer) {
            value->type = DLMS_DATA_ARRAY;
            value->array_val.items = p->buffer;
            value->array_val.count = p->buffer_count;
            value->array_val.capacity = p->buffer_capacity;
        }
        return DLMS_OK;
    }
    if (attr_index == 3) { *value = p->capture_objects; return DLMS_OK; }
    if (attr_index == 4) { *value = p->sort_method; return DLMS_OK; }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_profile_generic_create(dlms_profile_generic_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_PROFILE_GENERIC;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "ProfileGeneric";
    obj->base.get_attr = profile_generic_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 10;
    obj->base.method_count = 2;
    return DLMS_OK;
}

/* ===== COSEM Object Collection ===== */

int dlms_cosem_list_init(dlms_cosem_object_list_t *list) {
    if (!list) return DLMS_ERROR_INVALID;
    memset(list, 0, sizeof(*list));
    return DLMS_OK;
}

int dlms_cosem_list_add(dlms_cosem_object_list_t *list, dlms_cosem_object_t *obj) {
    if (!list || !obj) return DLMS_ERROR_INVALID;
    if (list->count >= DLMS_COSEM_MAX_OBJECTS) return DLMS_ERROR_BUFFER;
    list->objects[list->count++] = obj;
    return DLMS_OK;
}

dlms_cosem_object_t *dlms_cosem_list_find(const dlms_cosem_object_list_t *list,
                                           uint16_t class_id, const dlms_obis_t *ln) {
    if (!list) return NULL;
    for (uint16_t i = 0; i < list->count; i++) {
        if (list->objects[i]->class_id == class_id && ln &&
            dlms_obis_equals(&list->objects[i]->logical_name, ln))
            return list->objects[i];
    }
    return NULL;
}

dlms_cosem_object_t *dlms_cosem_list_find_by_ln(const dlms_cosem_object_list_t *list,
                                                 const dlms_obis_t *ln) {
    if (!list || !ln) return NULL;
    for (uint16_t i = 0; i < list->count; i++) {
        if (dlms_obis_equals(&list->objects[i]->logical_name, ln))
            return list->objects[i];
    }
    return NULL;
}
