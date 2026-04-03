/**
 * @file dlms_cosem.h
 * @brief COSEM Interface Classes (IC) - virtual dispatch via function pointers
 */
#ifndef DLMS_COSEM_H
#define DLMS_COSEM_H

#include "dlms_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DLMS_COSEM_MAX_ATTRS
#define DLMS_COSEM_MAX_ATTRS     16
#endif

#ifndef DLMS_COSEM_MAX_METHODS
#define DLMS_COSEM_MAX_METHODS   16
#endif

/* Forward declaration */
typedef struct dlms_cosem_object dlms_cosem_object_t;

/* COSEM Object - function pointer table (vtable pattern) */
struct dlms_cosem_object {
    uint16_t class_id;
    uint8_t version;
    dlms_obis_t logical_name;
    const char *name;

    /* Attribute access: get attribute value by attribute index (1-based) */
    int (*get_attr)(const dlms_cosem_object_t *obj, uint8_t attr_index,
                    dlms_value_t *value);

    /* Attribute access: set attribute value */
    int (*set_attr)(dlms_cosem_object_t *obj, uint8_t attr_index,
                    const dlms_value_t *value);

    /* Method invocation */
    int (*action)(dlms_cosem_object_t *obj, uint8_t method_index,
                  dlms_value_t *params, dlms_value_t *result);

    /* Optional: number of attributes/methods */
    uint8_t attr_count;
    uint8_t method_count;
};

/* ===== IC: Data ===== */
typedef struct {
    dlms_cosem_object_t base;
    dlms_value_t value;
} dlms_data_object_t;

int dlms_data_create(dlms_data_object_t *obj, const dlms_obis_t *ln);
int dlms_data_set_value(dlms_data_object_t *obj, const dlms_value_t *value);

/* ===== IC: Register ===== */
typedef struct {
    dlms_cosem_object_t base;
    dlms_value_t value;
    dlms_value_t scaler_unit;
} dlms_register_object_t;

int dlms_register_create(dlms_register_object_t *obj, const dlms_obis_t *ln);
int dlms_register_set_value(dlms_register_object_t *obj, const dlms_value_t *value);

/* ===== IC: Clock ===== */
typedef struct {
    dlms_cosem_object_t base;
    dlms_value_t datetime;
    int16_t timezone;
    uint8_t dst_status;
    uint8_t clock_status;
} dlms_clock_object_t;

int dlms_clock_create(dlms_clock_object_t *obj, const dlms_obis_t *ln);

/* ===== IC: SecuritySetup ===== */
typedef struct {
    dlms_cosem_object_t base;
    uint8_t security_suite;
    uint8_t enc_key[32];
    uint8_t enc_key_len;
    uint8_t auth_key[32];
    uint8_t auth_key_len;
    uint8_t system_title[8];
    uint8_t system_title_len;
} dlms_security_setup_object_t;

int dlms_security_setup_create(dlms_security_setup_object_t *obj, const dlms_obis_t *ln);

/* ===== IC: Association LN ===== */
typedef struct {
    dlms_cosem_object_t base;
    uint8_t auth_mechanism;
    dlms_value_t auth_value;
    const uint8_t *system_title;
    uint8_t system_title_len;
    uint8_t objects_list[256];
    uint16_t objects_list_len;
} dlms_association_ln_object_t;

int dlms_association_ln_create(dlms_association_ln_object_t *obj, const dlms_obis_t *ln);

/* ===== IC: Profile Generic ===== */
typedef struct {
    dlms_cosem_object_t base;
    dlms_value_t *buffer;
    uint16_t buffer_count;
    uint16_t buffer_capacity;
    dlms_value_t capture_objects;
    dlms_value_t sort_method;
} dlms_profile_generic_object_t;

int dlms_profile_generic_create(dlms_profile_generic_object_t *obj, const dlms_obis_t *ln);

/* ===== COSEM Object collection ===== */
#define DLMS_COSEM_MAX_OBJECTS 64

typedef struct {
    dlms_cosem_object_t *objects[DLMS_COSEM_MAX_OBJECTS];
    uint16_t count;
} dlms_cosem_object_list_t;

int dlms_cosem_list_init(dlms_cosem_object_list_t *list);
int dlms_cosem_list_add(dlms_cosem_object_list_t *list, dlms_cosem_object_t *obj);
dlms_cosem_object_t *dlms_cosem_list_find(const dlms_cosem_object_list_t *list,
                                           uint16_t class_id, const dlms_obis_t *ln);
dlms_cosem_object_t *dlms_cosem_list_find_by_ln(const dlms_cosem_object_list_t *list,
                                                 const dlms_obis_t *ln);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_COSEM_H */
