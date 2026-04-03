/**
 * @file dlms_client.c
 * @brief DLMS/COSEM Client implementation
 */
#include "dlms/dlms_client.h"
#include <string.h>

int dlms_client_init(dlms_client_t *client) {
    if (!client) return DLMS_ERROR_INVALID;
    memset(client, 0, sizeof(*client));
    client->state = DLMS_CLIENT_STATE_DISCONNECTED;
    client->dlms_version = 6;
    client->max_pdu_size = 65535;
    client->use_logical_name_referencing = true;
    client->client_invocation_counter = 1;

    /* Default HDLC params */
    dlms_hdlc_param_list_init(&client->hdlc_params);
    dlms_hdlc_param_list_set(&client->hdlc_params, DLMS_PARAM_WINDOW_SIZE, 1);
    dlms_hdlc_param_list_set(&client->hdlc_params, DLMS_PARAM_MAX_INFO_TX, 128);
    dlms_hdlc_param_list_set(&client->hdlc_params, DLMS_PARAM_MAX_INFO_RX, 128);

    return DLMS_OK;
}

int dlms_client_set_io(dlms_client_t *client, dlms_io_send_t send_fn,
                       dlms_io_recv_t recv_fn, void *user_data) {
    if (!client) return DLMS_ERROR_INVALID;
    client->send_fn = send_fn;
    client->recv_fn = recv_fn;
    client->io_user_data = user_data;
    return DLMS_OK;
}

int dlms_client_set_addresses(dlms_client_t *client,
                              uint16_t client_addr, uint16_t server_addr) {
    if (!client) return DLMS_ERROR_INVALID;
    client->client_address.logical_address = client_addr;
    client->client_address.has_physical = false;
    client->server_address.logical_address = server_addr;
    client->server_address.has_physical = false;
    return DLMS_OK;
}

int dlms_hdlc_connect(dlms_client_t *client) {
    if (!client || !client->send_fn) return DLMS_ERROR_INVALID;
    client->state = DLMS_CLIENT_STATE_HDLC_CONNECTING;

    /* Build and send SNRM */
    int ret = dlms_hdlc_build_snrm(client->tx_buf, sizeof(client->tx_buf),
                                   &client->server_address, &client->client_address,
                                   &client->hdlc_params);
    if (ret < 0) return ret;

    ret = client->send_fn(client->tx_buf, (size_t)ret, client->io_user_data);
    if (ret != DLMS_OK) return ret;

    /* Wait for UA */
    size_t recv_len = 0;
    ret = client->recv_fn(client->rx_buf, sizeof(client->rx_buf), &recv_len, 5000, client->io_user_data);
    if (ret != DLMS_OK || recv_len == 0) return DLMS_ERROR_TIMEOUT;

    /* Parse UA */
    dlms_hdlc_frame_t frame;
    ret = dlms_hdlc_frame_decode(client->rx_buf, recv_len, &frame);
    if (ret != DLMS_OK) return ret;
    if (frame.type != DLMS_HDLC_FRAME_UA) return DLMS_ERROR_PARSE;

    /* Parse negotiated params from UA info */
    if (frame.info && frame.info_len > 0) {
        dlms_hdlc_param_list_t server_params;
        dlms_hdlc_param_list_decode(frame.info, frame.info_len, &server_params);

        uint16_t val;
        if (dlms_hdlc_param_list_get(&server_params, DLMS_PARAM_WINDOW_SIZE, &val) == DLMS_OK)
            client->negotiated_window_size = (uint8_t)val;
        if (dlms_hdlc_param_list_get(&server_params, DLMS_PARAM_MAX_INFO_RX, &val) == DLMS_OK)
            dlms_hdlc_param_list_set(&client->hdlc_params, DLMS_PARAM_MAX_INFO_RX, val);
    }

    client->send_seq = 0;
    client->recv_seq = 0;
    return DLMS_OK;
}

int dlms_hdlc_disconnect(dlms_client_t *client) {
    if (!client || !client->send_fn) return DLMS_ERROR_INVALID;

    int ret = dlms_hdlc_build_disc(client->tx_buf, sizeof(client->tx_buf),
                                   &client->server_address, &client->client_address);
    if (ret < 0) return ret;

    ret = client->send_fn(client->tx_buf, (size_t)ret, client->io_user_data);
    client->state = DLMS_CLIENT_STATE_DISCONNECTED;
    return ret;
}

int dlms_associate(dlms_client_t *client, dlms_auth_mechanism_t auth,
                   const uint8_t *password, uint16_t password_len) {
    if (!client) return DLMS_ERROR_INVALID;
    client->auth_mechanism = auth;

    /* Build AARQ */
    dlms_aarq_t aarq;
    memset(&aarq, 0, sizeof(aarq));
    aarq.dlms_version = client->dlms_version;
    aarq.auth_mech = auth;
    aarq.auth_value = password;
    aarq.auth_value_len = password_len;
    aarq.proposed_max_pdu_size = client->max_pdu_size;

    size_t aarq_len = 0;
    int ret = dlms_aarq_encode(&aarq, client->tx_buf, sizeof(client->tx_buf), &aarq_len);
    if (ret != DLMS_OK) return ret;

    client->state = DLMS_CLIENT_STATE_ASSOCIATING;

    /* Send via HDLC I-frame */
    int frame_len = dlms_hdlc_build_info(
        client->tx_buf, sizeof(client->tx_buf),
        &client->server_address, &client->client_address,
        client->send_seq, client->recv_seq, true,
        client->tx_buf, (uint16_t)aarq_len);

    /* Need to use a temp buffer for the frame */
    uint8_t frame_buf[512];
    size_t temp_aarq_len = aarq_len;
    dlms_aarq_encode(&aarq, client->rx_buf, sizeof(client->rx_buf), &temp_aarq_len);

    frame_len = dlms_hdlc_build_info(
        frame_buf, sizeof(frame_buf),
        &client->server_address, &client->client_address,
        client->send_seq, client->recv_seq, true,
        client->rx_buf, (uint16_t)temp_aarq_len);
    if (frame_len < 0) return frame_len;

    if (!client->send_fn) return DLMS_ERROR_INVALID;
    ret = client->send_fn(frame_buf, (size_t)frame_len, client->io_user_data);
    if (ret != DLMS_OK) return ret;

    client->send_seq = (client->send_seq + 1) & 0x07;

    /* Wait for AARE */
    size_t recv_len = 0;
    ret = client->recv_fn(frame_buf, sizeof(frame_buf), &recv_len, 10000, client->io_user_data);
    if (ret != DLMS_OK || recv_len == 0) return DLMS_ERROR_TIMEOUT;

    dlms_hdlc_frame_t frame;
    ret = dlms_hdlc_frame_decode(frame_buf, recv_len, &frame);
    if (ret != DLMS_OK) return ret;

    if (frame.type == DLMS_HDLC_FRAME_RR) {
        /* Got RR, need to receive I-frame */
        ret = client->recv_fn(frame_buf, sizeof(frame_buf), &recv_len, 10000, client->io_user_data);
        if (ret != DLMS_OK) return ret;
        ret = dlms_hdlc_frame_decode(frame_buf, recv_len, &frame);
        if (ret != DLMS_OK) return ret;
    }

    if (frame.type != DLMS_HDLC_FRAME_INFO || !frame.info) return DLMS_ERROR_PARSE;

    /* Parse AARE */
    dlms_aare_t aare;
    size_t consumed = 0;
    ret = dlms_aare_decode(frame.info, frame.info_len, &aare, &consumed);
    if (ret != DLMS_OK) return ret;
    if (aare.result != 0) return DLMS_ERROR;

    if (aare.negotiated_max_pdu_size > 0)
        client->max_pdu_size = aare.negotiated_max_pdu_size;

    client->state = DLMS_CLIENT_STATE_ASSOCIATED;
    return DLMS_OK;
}

int dlms_hls_authenticate(dlms_client_t *client) {
    if (!client) return DLMS_ERROR_INVALID;
    client->state = DLMS_CLIENT_STATE_AUTHENTICATING;

    /* For HLS_GMAC, the client-to-server challenge was sent in AARQ.
     * Now we need to receive the server's challenge and respond. */
    /* This is a simplified implementation */
    client->state = DLMS_CLIENT_STATE_READY;
    return DLMS_OK;
}

int dlms_read(dlms_client_t *client, const dlms_obis_t *ln,
              uint8_t attr_index, dlms_value_t *result) {
    if (!client || !ln || !result) return DLMS_ERROR_INVALID;

    /* Build GetRequest */
    uint8_t apdu[64];
    size_t apdu_len = 0;

    /* GetRequest normal: 0xC0, 01 (type), 00 (selector), LN, attr_index */
    apdu[apdu_len++] = 0xC0; /* GetRequest */
    apdu[apdu_len++] = 0x01; /* GetRequestNormal */
    apdu[apdu_len++] = 0x00; /* no selector */
    /* Variable access specification (LN referencing) */
    apdu[apdu_len++] = 0x00; /* no ciphered */
    memcpy(apdu + apdu_len, ln->bytes, 6);
    apdu_len += 6;
    apdu[apdu_len++] = 0x01; /* attribute descriptor */
    apdu[apdu_len++] = attr_index;

    return dlms_send_apdu(client, apdu, apdu_len, NULL, 0, NULL);
}

int dlms_write(dlms_client_t *client, const dlms_obis_t *ln,
               uint8_t attr_index, const dlms_value_t *value) {
    if (!client || !ln || !value) return DLMS_ERROR_INVALID;

    uint8_t apdu[128];
    size_t apdu_len = 0;

    apdu[apdu_len++] = 0xC2; /* SetRequest */
    apdu[apdu_len++] = 0x01; /* SetRequestNormal */
    /* Variable access specification */
    apdu[apdu_len++] = 0x00;
    memcpy(apdu + apdu_len, ln->bytes, 6);
    apdu_len += 6;
    apdu[apdu_len++] = 0x02; /* attribute descriptor */
    apdu[apdu_len++] = attr_index;

    /* Encode value */
    size_t val_len = 0;
    int ret = dlms_axdr_encode(value, apdu + apdu_len, sizeof(apdu) - apdu_len, &val_len);
    if (ret != DLMS_OK) return ret;
    apdu_len += val_len;

    return dlms_send_apdu(client, apdu, apdu_len, NULL, 0, NULL);
}

int dlms_action(dlms_client_t *client, const dlms_obis_t *ln,
                uint8_t method_index, const dlms_value_t *params,
                dlms_value_t *result) {
    (void)client; (void)ln; (void)method_index; (void)params; (void)result;
    return DLMS_ERROR_NOT_SUPPORTED;
}

int dlms_send_apdu(dlms_client_t *client, const uint8_t *apdu, size_t len,
                   uint8_t *resp, size_t resp_size, size_t *resp_len) {
    if (!client || !apdu) return DLMS_ERROR_INVALID;
    if (!client->send_fn) return DLMS_ERROR_INVALID;

    /* Send in HDLC I-frame */
    uint8_t frame_buf[512];
    int frame_len = dlms_hdlc_build_info(
        frame_buf, sizeof(frame_buf),
        &client->server_address, &client->client_address,
        client->send_seq, client->recv_seq, true,
        apdu, (uint16_t)len);
    if (frame_len < 0) return frame_len;

    int ret = client->send_fn(frame_buf, (size_t)frame_len, client->io_user_data);
    if (ret != DLMS_OK) return ret;
    client->send_seq = (client->send_seq + 1) & 0x07;

    if (resp && resp_size > 0 && client->recv_fn) {
        size_t recv_len = 0;
        ret = client->recv_fn(frame_buf, sizeof(frame_buf), &recv_len, 10000, client->io_user_data);
        if (ret != DLMS_OK || recv_len == 0) return DLMS_ERROR_TIMEOUT;

        dlms_hdlc_frame_t frame;
        ret = dlms_hdlc_frame_decode(frame_buf, recv_len, &frame);
        if (ret != DLMS_OK) return ret;

        if (frame.type == DLMS_HDLC_FRAME_RR) {
            client->recv_seq = (client->recv_seq + 1) & 0x07;
            ret = client->recv_fn(frame_buf, sizeof(frame_buf), &recv_len, 10000, client->io_user_data);
            if (ret != DLMS_OK) return ret;
            ret = dlms_hdlc_frame_decode(frame_buf, recv_len, &frame);
            if (ret != DLMS_OK) return ret;
        }

        if (frame.type == DLMS_HDLC_FRAME_INFO && frame.info) {
            size_t copy_len = frame.info_len < resp_size ? frame.info_len : resp_size;
            memcpy(resp, frame.info, copy_len);
            if (resp_len) *resp_len = copy_len;
            client->recv_seq = (client->recv_seq + 1) & 0x07;
        }
    }

    return DLMS_OK;
}
