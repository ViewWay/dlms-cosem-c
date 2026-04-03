/**
 * @file dlms_core.h
 * @brief DLMS/COSEM core types and utilities
 *
 * Pure C99, no dynamic memory, embedded-friendly.
 */
#ifndef DLMS_CORE_H
#define DLMS_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration macros - override at compile time */
#ifndef DLMS_MAX_OBIS_STRING
#define DLMS_MAX_OBIS_STRING       32
#endif

#ifndef DLMS_MAX_DATETIME_STRING
#define DLMS_MAX_DATETIME_STRING   32
#endif

#ifndef DLMS_MAX_VALUE_DEPTH
#define DLMS_MAX_VALUE_DEPTH       8
#endif

/* Error codes */
#define DLMS_OK                    0
#define DLMS_ERROR                 -1
#define DLMS_ERROR_BUFFER          -2
#define DLMS_ERROR_PARSE           -3
#define DLMS_ERROR_INVALID         -4
#define DLMS_ERROR_NOT_SUPPORTED   -5
#define DLMS_ERROR_TIMEOUT         -6

/* OBIS code */
typedef struct {
    uint8_t bytes[6];
} dlms_obis_t;

int dlms_obis_parse(const uint8_t *buf, size_t len, dlms_obis_t *obis);
int dlms_obis_to_string(const dlms_obis_t *obis, char *buf, size_t buf_size);
int dlms_obis_from_string(const char *str, dlms_obis_t *obis);
bool dlms_obis_equals(const dlms_obis_t *a, const dlms_obis_t *b);
bool dlms_obis_is_wildcard(const dlms_obis_t *obis);

/* DLMS Data Types */
typedef enum {
    DLMS_DATA_NULL = 0,
    DLMS_DATA_ARRAY = 1,
    DLMS_DATA_STRUCTURE = 2,
    DLMS_DATA_BOOLEAN = 3,
    DLMS_DATA_BIT_STRING = 4,
    DLMS_DATA_DOUBLE_LONG = 5,        /* int32 */
    DLMS_DATA_DOUBLE_LONG_UNSIGNED = 6, /* uint32 */
    DLMS_DATA_OCTET_STRING = 9,
    DLMS_DATA_VISIBLE_STRING = 10,
    DLMS_DATA_UTF8_STRING = 12,
    DLMS_DATA_BCD = 13,
    DLMS_DATA_INTEGER = 15,           /* int8 */
    DLMS_DATA_LONG = 16,              /* int16 */
    DLMS_DATA_UNSIGNED_INTEGER = 17,  /* uint8 */
    DLMS_DATA_UNSIGNED_LONG = 18,     /* uint16 */
    DLMS_DATA_COMPACT_ARRAY = 19,
    DLMS_DATA_LONG64 = 20,            /* int64 */
    DLMS_DATA_UNSIGNED_LONG64 = 21,   /* uint64 */
    DLMS_DATA_ENUM = 22,
    DLMS_DATA_FLOAT32 = 23,
    DLMS_DATA_FLOAT64 = 24,
    DLMS_DATA_DATETIME = 25,
    DLMS_DATA_DATE = 26,
    DLMS_DATA_TIME = 27,
    DLMS_DATA_DONT_CARE = 255
} dlms_data_type_t;

/* DLMS Value - tagged union for all data types */
typedef struct dlms_value {
    dlms_data_type_t type;
    union {
        bool boolean_val;
        int8_t int8_val;
        uint8_t uint8_val;
        int16_t int16_val;
        uint16_t uint16_val;
        int32_t int32_val;
        uint32_t uint32_val;
        int64_t int64_val;
        uint64_t uint64_val;
        float float32_val;
        double float64_val;
        struct {
            const uint8_t *data;
            uint16_t len;
        } octet_string;
        struct {
            const char *data;
            uint16_t len;
        } string_val;
        struct {
            const uint8_t *data;
            uint16_t len;
        } bit_string;
        struct {
            uint16_t year;
            uint8_t month;
            uint8_t day;
            uint8_t hour;
            uint8_t minute;
            uint8_t second;
            uint8_t hundredths;
            int16_t deviation;
            uint8_t clock_status;
        } datetime;
        struct {
            uint16_t year;
            uint8_t month;
            uint8_t day;
            uint8_t day_of_week;
        } date;
        struct {
            uint8_t hour;
            uint8_t minute;
            uint8_t second;
            uint8_t hundredths;
        } time_val;
        struct {
            struct dlms_value *items;
            uint16_t count;
            uint16_t capacity;
        } array_val;
    };
} dlms_value_t;

/* Byte buffer */
typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} dlms_buffer_t;

void dlms_buffer_init(dlms_buffer_t *buf, uint8_t *data, size_t cap);
int dlms_buffer_append(dlms_buffer_t *buf, const uint8_t *data, size_t len);
int dlms_buffer_append_byte(dlms_buffer_t *buf, uint8_t byte);
void dlms_buffer_clear(dlms_buffer_t *buf);
int dlms_buffer_remaining(const dlms_buffer_t *buf);

/* Variable-length integer encoding (AXDR/DLMS) */
int dlms_varint_encode(uint32_t value, uint8_t *dst, size_t dst_size, size_t *written);
int dlms_varint_decode(const uint8_t *data, size_t len, uint32_t *value, size_t *consumed);

/* Utility functions */
const char *dlms_data_type_name(dlms_data_type_t type);
uint16_t dlms_data_type_fixed_length(dlms_data_type_t type);
int dlms_reverse_bits8(uint8_t b);
uint16_t dlms_reverse_bits16(uint16_t v);

/* Value init/free helpers (for stack-allocated values with dynamic children) */
void dlms_value_init(dlms_value_t *v);
void dlms_value_set_null(dlms_value_t *v);
void dlms_value_set_bool(dlms_value_t *v, bool val);
void dlms_value_set_int8(dlms_value_t *v, int8_t val);
void dlms_value_set_uint8(dlms_value_t *v, uint8_t val);
void dlms_value_set_int16(dlms_value_t *v, int16_t val);
void dlms_value_set_uint16(dlms_value_t *v, uint16_t val);
void dlms_value_set_int32(dlms_value_t *v, int32_t val);
void dlms_value_set_uint32(dlms_value_t *v, uint32_t val);
void dlms_value_set_int64(dlms_value_t *v, int64_t val);
void dlms_value_set_uint64(dlms_value_t *v, uint64_t val);
void dlms_value_set_enum(dlms_value_t *v, uint8_t val);
void dlms_value_set_octet(dlms_value_t *v, const uint8_t *data, uint16_t len);
void dlms_value_set_string(dlms_value_t *v, const char *data, uint16_t len);
void dlms_value_set_float32(dlms_value_t *v, float val);
void dlms_value_set_float64(dlms_value_t *v, double val);

/* Data access result codes */
typedef enum {
    DLMS_ACCESS_SUCCESS = 0,
    DLMS_ACCESS_HARDWARE_FAULT = 1,
    DLMS_ACCESS_TEMPORARY_FAILURE = 2,
    DLMS_ACCESS_READ_WRITE_DENIED = 3,
    DLMS_ACCESS_OBJECT_UNDEFINED = 4,
    DLMS_ACCESS_OBJECT_CLASS_INCONSISTENT = 9,
    DLMS_ACCESS_OBJECT_UNAVAILABLE = 11,
    DLMS_ACCESS_TYPE_UNMATCHED = 12,
    DLMS_ACCESS_SCOPE_VIOLATED = 13,
    DLMS_ACCESS_DATA_BLOCK_UNAVAILABLE = 14,
    DLMS_ACCESS_OTHER_REASON = 250
} dlms_access_result_t;

/* Cosem Interface Class IDs */
typedef enum {
    DLMS_IC_DATA = 1,
    DLMS_IC_REGISTER = 3,
    DLMS_IC_EXTENDED_REGISTER = 4,
    DLMS_IC_DEMAND_REGISTER = 5,
    DLMS_IC_REGISTER_ACTIVATION = 6,
    DLMS_IC_PROFILE_GENERIC = 7,
    DLMS_IC_CLOCK = 8,
    DLMS_IC_SCRIPT_TABLE = 9,
    DLMS_IC_SCHEDULE = 10,
    DLMS_IC_SPECIAL_DAYS_TABLE = 11,
    DLMS_IC_ASSOCIATION_SN = 12,
    DLMS_IC_ASSOCIATION_LN = 15,
    DLMS_IC_SAP_ASSIGNMENT = 17,
    DLMS_IC_IMAGE_TRANSFER = 18,
    DLMS_IC_IEC_LOCAL_PORT_SETUP = 19,
    DLMS_IC_ACTIVITY_CALENDAR = 20,
    DLMS_IC_REGISTER_MONITOR = 21,
    DLMS_IC_SINGLE_ACTION_SCHEDULE = 22,
    DLMS_IC_PUSH = 40,
    DLMS_IC_TCP_UDP_SETUP = 41,
    DLMS_IC_SECURITY_SETUP = 64,
    DLMS_IC_DISCONNECT_CONTROL = 70,
    DLMS_IC_LIMITER = 71,
    DLMS_IC_ACCOUNT = 111,
    DLMS_IC_CREDIT = 112,
    DLMS_IC_CHARGE = 113,
} dlms_ic_class_t;

/* Authentication mechanism */
typedef enum {
    DLMS_AUTH_NONE = 0,
    DLMS_AUTH_LLS = 1,
    DLMS_AUTH_HLS = 2,
    DLMS_AUTH_HLS_MD5 = 3,
    DLMS_AUTH_HLS_SHA1 = 4,
    DLMS_AUTH_HLS_GMAC = 5,
    DLMS_AUTH_HLS_SHA256 = 6,
    DLMS_AUTH_HLS_ECDSA = 7
} dlms_auth_mechanism_t;

#ifdef __cplusplus
}
#endif

#endif /* DLMS_CORE_H */
