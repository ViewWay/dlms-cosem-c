/**
 * @file dlms_client.h
 * @brief DLMS/COSEM Client - association, read, write, action
 */
#ifndef DLMS_CLIENT_H
#define DLMS_CLIENT_H

#include "dlms_core.h"
#include "dlms_hdlc.h"
#include "dlms_axdr.h"
#include "dlms_asn1.h"
#include "dlms_security.h"
#include "dlms_cosem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Transport types */
typedef enum {
    DLMS_TRANSPORT_HDLC_SERIAL,
    DLMS_TRANSPORT_HDLC_TCP,
    DLMS_TRANSPORT_TCP,
    DLMS_TRANSPORT_WRAPPER
} dlms_transport_type_t;

/* Client connection state */
typedef enum {
    DLMS_CLIENT_STATE_DISCONNECTED,
    DLMS_CLIENT_STATE_HDLC_CONNECTING,
    DLMS_CLIENT_STATE_ASSOCIATING,
    DLMS_CLIENT_STATE_ASSOCIATED,
    DLMS_CLIENT_STATE_AUTHENTICATING,
    DLMS_CLIENT_STATE_READY,
    DLMS_CLIENT_STATE_ERROR
} dlms_client_state_t;

/* IO callback for transport abstraction */
typedef int (*dlms_io_send_t)(const uint8_t *data, size_t len, void *user_data);
typedef int (*dlms_io_recv_t)(uint8_t *data, size_t max_len, size_t *received, int timeout_ms, void *user_data);

/* DLMS Client */
typedef struct {
    dlms_client_state_t state;
    dlms_transport_type_t transport;

    /* HDLC layer */
    dlms_hdlc_address_t client_address;
    dlms_hdlc_address_t server_address;
    dlms_hdlc_param_list_t hdlc_params;
    uint8_t send_seq;
    uint8_t recv_seq;
    uint8_t negotiated_window_size;

    /* Association */
    uint8_t dlms_version;
    dlms_auth_mechanism_t auth_mechanism;
    uint16_t max_pdu_size;
    uint16_t conformance;
    bool use_logical_name_referencing;

    /* Security */
    uint8_t security_suite;
    uint8_t encryption_key[32];
    uint8_t encryption_key_len;
    uint8_t authentication_key[32];
    uint8_t authentication_key_len;
    uint8_t client_system_title[8];
    uint8_t client_system_title_len;
    uint8_t server_system_title[8];
    uint8_t server_system_title_len;
    uint32_t client_invocation_counter;
    uint32_t server_invocation_counter;
    uint8_t client_challenge[64];
    uint8_t client_challenge_len;
    uint8_t server_challenge[64];
    uint8_t server_challenge_len;

    /* Transport IO */
    dlms_io_send_t send_fn;
    dlms_io_recv_t recv_fn;
    void *io_user_data;

    /* Internal buffers */
    uint8_t tx_buf[512];
    uint8_t rx_buf[512];
    dlms_hdlc_parser_t *parser;
} dlms_client_t;

/* Initialize client */
int dlms_client_init(dlms_client_t *client);

/* Set transport IO callbacks */
int dlms_client_set_io(dlms_client_t *client, dlms_io_send_t send_fn,
                       dlms_io_recv_t recv_fn, void *user_data);

/* Set addresses */
int dlms_client_set_addresses(dlms_client_t *client,
                              uint16_t client_addr, uint16_t server_addr);

/* HDLC connection */
int dlms_hdlc_connect(dlms_client_t *client);
int dlms_hdlc_disconnect(dlms_client_t *client);

/* Association (AARQ/AARE) */
int dlms_associate(dlms_client_t *client, dlms_auth_mechanism_t auth,
                   const uint8_t *password, uint16_t password_len);

/* HLS authentication */
int dlms_hls_authenticate(dlms_client_t *client);

/* Read attribute */
int dlms_read(dlms_client_t *client, const dlms_obis_t *ln,
              uint8_t attr_index, dlms_value_t *result);

/* Write attribute */
int dlms_write(dlms_client_t *client, const dlms_obis_t *ln,
               uint8_t attr_index, const dlms_value_t *value);

/* Action (method) */
int dlms_action(dlms_client_t *client, const dlms_obis_t *ln,
                uint8_t method_index, const dlms_value_t *params,
                dlms_value_t *result);

/* Raw APDU send/receive */
int dlms_send_apdu(dlms_client_t *client, const uint8_t *apdu, size_t len,
                   uint8_t *resp, size_t resp_size, size_t *resp_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_CLIENT_H */
