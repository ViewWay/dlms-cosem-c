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
        if (value->type == DLMS_DATA_OCTET_STRING && value->u.octet_string.len >= 3) {
            c->timezone = (int16_t)((value->u.octet_string.data[0] << 8) | value->u.octet_string.data[1]);
            c->dst_status = value->u.octet_string.data[2];
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
            value->u.array_val.items = p->buffer;
            value->u.array_val.count = p->buffer_count;
            value->u.array_val.capacity = p->buffer_capacity;
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

/* ===== IC: Demand ===== */

static int demand_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_demand_object_t *d = (dlms_demand_object_t *)obj;
    if (attr_index == 2) { *value = d->current_average_value; return DLMS_OK; }
    if (attr_index == 3) { *value = d->last_average_value; return DLMS_OK; }
    if (attr_index == 4) { *value = d->scaler_unit; return DLMS_OK; }
    if (attr_index == 5) { dlms_value_set_uint32(value, d->period); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, d->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_demand_create(dlms_demand_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_DEMAND;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Demand";
    obj->base.get_attr = demand_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 5;
    obj->base.method_count = 0;
    dlms_value_set_int32(&obj->current_average_value, 0);
    dlms_value_set_int32(&obj->last_average_value, 0);
    obj->period = 900;
    return DLMS_OK;
}

/* ===== IC: Register Monitor ===== */

static int register_monitor_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_register_monitor_object_t *m = (dlms_register_monitor_object_t *)obj;
    if (attr_index == 2) { *value = m->monitored_value; return DLMS_OK; }
    if (attr_index == 3) { *value = m->threshold; return DLMS_OK; }
    if (attr_index == 4) { *value = m->scaler_unit; return DLMS_OK; }
    if (attr_index == 5) { dlms_value_set_uint8(value, m->monitor_status); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, m->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_register_monitor_create(dlms_register_monitor_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_REGISTER_MONITOR;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "RegisterMonitor";
    obj->base.get_attr = register_monitor_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 5;
    obj->base.method_count = 2;
    dlms_value_set_int32(&obj->monitored_value, 0);
    dlms_value_set_int32(&obj->threshold, 0);
    obj->monitor_status = 0;
    return DLMS_OK;
}

/* ===== IC: Disconnect Control ===== */

static int disconnect_control_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_disconnect_control_object_t *d = (dlms_disconnect_control_object_t *)obj;
    if (attr_index == 2) { dlms_value_set_uint8(value, d->control_state); return DLMS_OK; }
    if (attr_index == 3) { *value = d->control_mode; return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, d->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_disconnect_control_create(dlms_disconnect_control_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_DISCONNECT_CONTROL;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "DisconnectControl";
    obj->base.get_attr = disconnect_control_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 4;
    obj->base.method_count = 3;
    obj->control_state = 0;
    return DLMS_OK;
}

/* ===== IC: Limiter ===== */

static int limiter_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_limiter_object_t *l = (dlms_limiter_object_t *)obj;
    if (attr_index == 2) { *value = l->emergency_cutoff; return DLMS_OK; }
    if (attr_index == 3) { *value = l->monitor_mode; return DLMS_OK; }
    if (attr_index == 4) { dlms_value_set_uint8(value, l->limiter_status); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, l->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_limiter_create(dlms_limiter_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_LIMITER;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Limiter";
    obj->base.get_attr = limiter_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 5;
    obj->base.method_count = 2;
    dlms_value_set_int32(&obj->emergency_cutoff, 0);
    obj->limiter_status = 0;
    return DLMS_OK;
}

/* ===== IC: Account ===== */

static int account_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_account_object_t *a = (dlms_account_object_t *)obj;
    if (attr_index == 2) { *value = a->current_credit; return DLMS_OK; }
    if (attr_index == 3) { *value = a->credit_status; return DLMS_OK; }
    if (attr_index == 4) { dlms_value_set_uint8(value, a->account_status); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, a->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_account_create(dlms_account_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_ACCOUNT;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Account";
    obj->base.get_attr = account_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 5;
    obj->base.method_count = 0;
    dlms_value_set_int32(&obj->current_credit, 0);
    obj->account_status = 0;
    return DLMS_OK;
}

/* ===== IC: Day Profile ===== */

static int day_profile_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_day_profile_object_t *d = (dlms_day_profile_object_t *)obj;
    if (attr_index == 2) { *value = d->calendar_name; return DLMS_OK; }
    if (attr_index == 3) { dlms_value_set_uint8(value, d->seasons); return DLMS_OK; }
    if (attr_index == 4) { dlms_value_set_uint8(value, d->week_profiles); return DLMS_OK; }
    if (attr_index == 5) { dlms_value_set_uint8(value, d->day_profiles); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, d->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_day_profile_create(dlms_day_profile_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_ACTIVITY_CALENDAR;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "DayProfile";
    obj->base.get_attr = day_profile_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 6;
    obj->base.method_count = 1;
    obj->seasons = 0;
    obj->week_profiles = 0;
    obj->day_profiles = 0;
    return DLMS_OK;
}

/* ===== IC: Week Profile ===== */

static int week_profile_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_week_profile_object_t *w = (dlms_week_profile_object_t *)obj;
    if (attr_index == 2) { *value = w->profile_name; return DLMS_OK; }
    if (attr_index == 3) { dlms_value_set_uint8(value, w->monday); return DLMS_OK; }
    if (attr_index == 4) { dlms_value_set_uint8(value, w->tuesday); return DLMS_OK; }
    if (attr_index == 5) { dlms_value_set_uint8(value, w->wednesday); return DLMS_OK; }
    if (attr_index == 6) { dlms_value_set_uint8(value, w->thursday); return DLMS_OK; }
    if (attr_index == 7) { dlms_value_set_uint8(value, w->friday); return DLMS_OK; }
    if (attr_index == 8) { dlms_value_set_uint8(value, w->saturday); return DLMS_OK; }
    if (attr_index == 9) { dlms_value_set_uint8(value, w->sunday); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, w->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_week_profile_create(dlms_week_profile_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_SPECIAL_DAYS_TABLE;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "WeekProfile";
    obj->base.get_attr = week_profile_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 9;
    obj->base.method_count = 0;
    obj->monday = 0;
    obj->tuesday = 0;
    obj->wednesday = 0;
    obj->thursday = 0;
    obj->friday = 0;
    obj->saturday = 0;
    obj->sunday = 0;
    return DLMS_OK;
}

/* ===== IC: Extended Register ===== */

static int extended_register_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_extended_register_object_t *r = (dlms_extended_register_object_t *)obj;
    if (attr_index == 2) { *value = r->value; return DLMS_OK; }
    if (attr_index == 3) { *value = r->scaler_unit; return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, r->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_extended_register_create(dlms_extended_register_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_EXTENDED_REGISTER;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "ExtendedRegister";
    obj->base.get_attr = extended_register_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 3;
    obj->base.method_count = 0;
    dlms_value_set_int32(&obj->value, 0);
    return DLMS_OK;
}

/* ===== IC: Demand Register ===== */

static int demand_register_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_demand_register_object_t *d = (dlms_demand_register_object_t *)obj;
    if (attr_index == 2) { *value = d->current_value; return DLMS_OK; }
    if (attr_index == 3) { *value = d->scaler_unit; return DLMS_OK; }
    if (attr_index == 4) { dlms_value_set_uint32(value, d->period); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, d->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_demand_register_create(dlms_demand_register_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_DEMAND_REGISTER;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "DemandRegister";
    obj->base.get_attr = demand_register_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 4;
    obj->base.method_count = 0;
    dlms_value_set_int32(&obj->current_value, 0);
    obj->period = 900;
    return DLMS_OK;
}

/* ===== IC: Credit ===== */

static int credit_get_attr(const dlms_cosem_object_t *obj, uint8_t attr_index, dlms_value_t *value) {
    dlms_credit_object_t *c = (dlms_credit_object_t *)obj;
    if (attr_index == 2) { *value = c->credit_amount; return DLMS_OK; }
    if (attr_index == 3) { dlms_value_set_uint8(value, c->credit_status); return DLMS_OK; }
    if (attr_index == 1) {
        uint8_t ln[6]; memcpy(ln, c->base.logical_name.bytes, 6);
        dlms_value_set_octet(value, ln, 6);
        return DLMS_OK;
    }
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_credit_create(dlms_credit_object_t *obj, const dlms_obis_t *ln) {
    if (!obj || !ln) return DLMS_ERROR_INVALID;
    memset(obj, 0, sizeof(*obj));
    obj->base.class_id = DLMS_IC_CREDIT;
    obj->base.version = 0;
    obj->base.logical_name = *ln;
    obj->base.name = "Credit";
    obj->base.get_attr = credit_get_attr;
    obj->base.set_attr = NULL;
    obj->base.action = NULL;
    obj->base.attr_count = 3;
    obj->base.method_count = 0;
    dlms_value_set_int32(&obj->credit_amount, 0);
    obj->credit_status = 0;
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
