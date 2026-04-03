/**
 * @file dlms_asn1.c
 * @brief BER-TLV encoding for AARQ/AARE
 */
#include "dlms/dlms_asn1.h"
#include <string.h>

/* ===== BER encoding helpers ===== */

int dlms_ber_encode_tag(uint8_t tag, uint8_t *dst, size_t dst_size, size_t *written) {
    if (!dst || !written) return DLMS_ERROR_INVALID;
    if (dst_size < 1) return DLMS_ERROR_BUFFER;
    dst[0] = tag;
    *written = 1;
    return DLMS_OK;
}

int dlms_ber_decode_tag(const uint8_t *data, size_t len, dlms_ber_header_t *header, size_t *consumed) {
    if (!data || !header || !consumed || len < 1) return DLMS_ERROR_INVALID;
    *consumed = 0;
    header->tag = data[0];
    header->constructed = (data[0] & 0x20) != 0;

    /* Decode length */
    size_t lc = 0;
    int ret = dlms_ber_decode_length(data + 1, len - 1, &header->length, &lc);
    if (ret != DLMS_OK) return ret;
    *consumed = 1 + lc;
    return DLMS_OK;
}

int dlms_ber_encode_length(uint32_t length, uint8_t *dst, size_t dst_size, size_t *written) {
    if (!dst || !written) return DLMS_ERROR_INVALID;
    if (length <= 0x7F) {
        if (dst_size < 1) return DLMS_ERROR_BUFFER;
        dst[0] = (uint8_t)length;
        *written = 1;
    } else if (length <= 0xFF) {
        if (dst_size < 2) return DLMS_ERROR_BUFFER;
        dst[0] = 0x81;
        dst[1] = (uint8_t)length;
        *written = 2;
    } else if (length <= 0xFFFF) {
        if (dst_size < 3) return DLMS_ERROR_BUFFER;
        dst[0] = 0x82;
        dst[1] = (uint8_t)(length >> 8);
        dst[2] = (uint8_t)(length & 0xFF);
        *written = 3;
    } else {
        return DLMS_ERROR_INVALID;
    }
    return DLMS_OK;
}

int dlms_ber_decode_length(const uint8_t *data, size_t len, uint32_t *length, size_t *consumed) {
    if (!data || !length || !consumed || len < 1) return DLMS_ERROR_INVALID;
    if (!(data[0] & 0x80)) {
        *length = data[0];
        *consumed = 1;
        return DLMS_OK;
    }
    uint8_t nbytes = data[0] & 0x7F;
    if (nbytes == 0 || (size_t)(1 + nbytes) > len) return DLMS_ERROR_PARSE;
    uint32_t val = 0;
    for (uint8_t i = 0; i < nbytes; i++) {
        val = (val << 8) | data[1 + i];
    }
    *length = val;
    *consumed = 1 + nbytes;
    return DLMS_OK;
}

/* ===== Initiate Request (A-XDR encoded) ===== */

int dlms_initiate_request_encode(const dlms_initiate_request_t *req,
                                 uint8_t *dst, size_t dst_size, size_t *written) {
    if (!req || !dst || !written) return DLMS_ERROR_INVALID;
    *written = 0;
    size_t pos = 0;

    /* Protocol version */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 0x00; /* default indicator */

    /* Conformance (default) */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 0x00; /* default indicator */

    /* DLMS version */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 0x01;
    dst[pos++] = req->dlms_version;

    /* Authentication mechanism */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 0x01;
    dst[pos++] = (uint8_t)req->auth_mech;

    /* Client to server challenge (optional) */
    if (req->c_to_s_challenge && req->c_to_s_challenge_len > 0) {
        if (pos + 1 >= dst_size) return DLMS_ERROR_BUFFER;
        dst[pos++] = 0x01;
        if (pos + 1 >= dst_size) return DLMS_ERROR_BUFFER;
        dst[pos++] = (uint8_t)req->c_to_s_challenge_len;
        if (pos + req->c_to_s_challenge_len > dst_size) return DLMS_ERROR_BUFFER;
        memcpy(dst + pos, req->c_to_s_challenge, req->c_to_s_challenge_len);
        pos += req->c_to_s_challenge_len;
    } else {
        if (pos >= dst_size) return DLMS_ERROR_BUFFER;
        dst[pos++] = 0x00;
    }

    /* Max PDU size (default for now) */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = 0x00;

    *written = pos;
    return DLMS_OK;
}

int dlms_initiate_response_decode(const uint8_t *data, size_t len,
                                  dlms_initiate_response_t *resp, size_t *consumed) {
    if (!data || !resp || !consumed || len < 3) return DLMS_ERROR_INVALID;
    *consumed = 0;
    size_t pos = 0;

    /* Conformance (default or value) */
    if (data[pos] == 0x00) { pos++; }
    else if (data[pos] == 0x01) { pos++; pos += 4; } /* 4 bytes conformance */
    else return DLMS_ERROR_PARSE;

    /* DLMS version */
    if (pos + 2 > len) return DLMS_ERROR_BUFFER;
    if (data[pos] == 0x00) { pos++; }
    else if (data[pos] == 0x01) { pos++; resp->dlms_version = data[pos++]; }
    else return DLMS_ERROR_PARSE;

    /* Authentication */
    if (pos + 2 > len) return DLMS_ERROR_BUFFER;
    if (data[pos] == 0x00) { pos++; }
    else if (data[pos] == 0x01) { pos++; resp->auth_mech = (dlms_auth_mechanism_t)data[pos++]; }
    else return DLMS_ERROR_PARSE;

    /* Server system title (optional) */
    resp->server_system_title_len = 0;
    if (pos < len) {
        if (data[pos] == 0x00) { pos++; }
        else if (data[pos] == 0x01) {
            pos++;
            if (pos + 1 > len) return DLMS_ERROR_BUFFER;
            uint8_t stlen = data[pos++];
            if (pos + stlen > len) return DLMS_ERROR_BUFFER;
            if (stlen > 8) stlen = 8;
            memcpy(resp->server_system_title, data + pos, stlen);
            resp->server_system_title_len = stlen;
            pos += stlen;
        }
    }

    /* Negotiated PDU size */
    if (pos < len) {
        if (data[pos] == 0x00) { pos++; }
        else if (data[pos] == 0x01) {
            pos++;
            if (pos + 2 > len) return DLMS_ERROR_BUFFER;
            resp->negotiated_max_pdu_size = ((uint16_t)data[pos] << 8) | data[pos + 1];
            pos += 2;
        }
    }

    *consumed = pos;
    return DLMS_OK;
}

/* ===== AARQ encoding ===== */

int dlms_aarq_encode(const dlms_aarq_t *aarq, uint8_t *dst, size_t dst_size, size_t *written) {
    if (!aarq || !dst || !written) return DLMS_ERROR_INVALID;
    *written = 0;

    /* Build application context name (OID for LN referencing: 1.0.0.41.1.0.3) */
    uint8_t app_ctx[] = { 0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01 };

    /* Build initiate request */
    uint8_t init_req[128];
    size_t init_len = 0;
    dlms_initiate_request_t req;
    memset(&req, 0, sizeof(req));
    req.dlms_version = aarq->dlms_version;
    req.auth_mech = aarq->auth_mech;
    req.auth_value = aarq->auth_value;
    req.auth_value_len = aarq->auth_value_len;
    req.c_to_s_challenge = aarq->auth_value;
    req.c_to_s_challenge_len = aarq->auth_value_len;
    req.proposed_max_pdu_size = aarq->proposed_max_pdu_size;

    int ret = dlms_initiate_request_encode(&req, init_req, sizeof(init_req), &init_len);
    if (ret != DLMS_OK) return ret;

    /* Build user information */
    uint8_t user_info[256];
    size_t ui_pos = 0;

    /* Auth value (optional) */
    if (aarq->auth_value && aarq->auth_value_len > 0) {
        user_info[ui_pos++] = 0x80; /* context tag 0: calling authentication value */
        user_info[ui_pos++] = (uint8_t)aarq->auth_value_len;
        memcpy(user_info + ui_pos, aarq->auth_value, aarq->auth_value_len);
        ui_pos += aarq->auth_value_len;
    }

    /* Initiate request (context tag 1) */
    user_info[ui_pos++] = 0x81;
    size_t ir_len_enc = 0;
    ret = dlms_ber_encode_length((uint32_t)init_len, user_info + ui_pos, sizeof(user_info) - ui_pos, &ir_len_enc);
    if (ret != DLMS_OK) return ret;
    ui_pos += ir_len_enc;
    memcpy(user_info + ui_pos, init_req, init_len);
    ui_pos += init_len;

    /* Wrap user info in OCTET STRING (context 2) */
    uint8_t user_info_wrapped[260];
    size_t uiw_pos = 0;
    user_info_wrapped[uiw_pos++] = 0xBE; /* context tag 14 constructed */
    size_t ui_len_enc = 0;
    ret = dlms_ber_encode_length((uint32_t)ui_pos, user_info_wrapped + uiw_pos, sizeof(user_info_wrapped) - uiw_pos, &ui_len_enc);
    if (ret != DLMS_OK) return ret;
    uiw_pos += ui_len_enc;
    memcpy(user_info_wrapped + uiw_pos, user_info, ui_pos);
    uiw_pos += ui_pos;

    /* Build full AARQ */
    size_t pos = 0;

    /* AARQ tag */
    if (pos >= dst_size) return DLMS_ERROR_BUFFER;
    dst[pos++] = DLMS_ASN1_TAG_AARQ;

    /* Calculate total content length */
    size_t content_len = sizeof(app_ctx) + uiw_pos;
    size_t cl_enc = 0;
    ret = dlms_ber_encode_length((uint32_t)content_len, dst + pos, dst_size - pos, &cl_enc);
    if (ret != DLMS_OK) return ret;
    pos += cl_enc;

    /* Application context */
    if (pos + sizeof(app_ctx) > dst_size) return DLMS_ERROR_BUFFER;
    memcpy(dst + pos, app_ctx, sizeof(app_ctx));
    pos += sizeof(app_ctx);

    /* User information */
    if (pos + uiw_pos > dst_size) return DLMS_ERROR_BUFFER;
    memcpy(dst + pos, user_info_wrapped, uiw_pos);
    pos += uiw_pos;

    *written = pos;
    return DLMS_OK;
}

int dlms_aarq_decode(const uint8_t *data, size_t len, dlms_aarq_t *aarq, size_t *consumed) {
    if (!data || !aarq || !consumed || len < 2) return DLMS_ERROR_INVALID;
    *consumed = 0;

    if (data[0] != DLMS_ASN1_TAG_AARQ) return DLMS_ERROR_PARSE;

    dlms_ber_header_t header;
    size_t hc = 0;
    int ret = dlms_ber_decode_tag(data, len, &header, &hc);
    if (ret != DLMS_OK) return ret;

    size_t pos = hc;
    if (pos + header.length > len) return DLMS_ERROR_BUFFER;

    /* Skip application context name */
    if (pos < len && data[pos] == 0x06) {
        uint32_t oid_len = 0;
        size_t lc = 0;
        ret = dlms_ber_decode_length(data + pos + 1, len - pos - 1, &oid_len, &lc);
        if (ret != DLMS_OK) return ret;
        pos += 1 + lc + oid_len;
    }

    /* Parse user information */
    if (pos < len && data[pos] == 0xBE) {
        uint32_t ui_len = 0;
        size_t lc = 0;
        ret = dlms_ber_decode_length(data + pos + 1, len - pos - 1, &ui_len, &lc);
        if (ret != DLMS_OK) return ret;
        pos += 1 + lc;
        size_t ui_end = pos + ui_len;

        while (pos < ui_end) {
            uint8_t tag = data[pos++];
            if (tag == 0x80) {
                /* Calling authentication value */
                if (pos >= ui_end) break;
                uint8_t alen = data[pos++];
                aarq->auth_value = data + pos;
                aarq->auth_value_len = alen;
                pos += alen;
            } else if (tag == 0x81) {
                /* Initiate request */
                uint32_t ir_len = 0;
                size_t lc2 = 0;
                ret = dlms_ber_decode_length(data + pos, ui_end - pos, &ir_len, &lc2);
                if (ret != DLMS_OK) break;
                pos += lc2;
                if (pos + ir_len > ui_end) break;
                /* Parse initiate request fields */
                size_t ir_pos = 0;
                const uint8_t *ir = data + pos;
                /* Skip conformance default */
                if (ir_pos < ir_len && ir[ir_pos] == 0x00) ir_pos++;
                /* DLMS version */
                if (ir_pos + 1 < ir_len && ir[ir_pos] == 0x01) {
                    ir_pos++;
                    aarq->dlms_version = ir[ir_pos++];
                }
                /* Auth mechanism */
                if (ir_pos + 1 < ir_len && ir[ir_pos] == 0x01) {
                    ir_pos++;
                    aarq->auth_mech = (dlms_auth_mechanism_t)ir[ir_pos++];
                }
                pos += ir_len;
            } else {
                break;
            }
        }
    }

    *consumed = hc + header.length;
    return DLMS_OK;
}

int dlms_aare_decode(const uint8_t *data, size_t len, dlms_aare_t *aare, size_t *consumed) {
    if (!data || !aare || !consumed || len < 2) return DLMS_ERROR_INVALID;
    *consumed = 0;
    memset(aare, 0, sizeof(*aare));

    if (data[0] != DLMS_ASN1_TAG_AARE) return DLMS_ERROR_PARSE;

    dlms_ber_header_t header;
    size_t hc = 0;
    int ret = dlms_ber_decode_tag(data, len, &header, &hc);
    if (ret != DLMS_OK) return ret;

    size_t pos = hc;
    if (pos + header.length > len) return DLMS_ERROR_BUFFER;

    /* Application context name */
    if (pos < len && data[pos] == 0x06) {
        uint32_t oid_len = 0;
        size_t lc = 0;
        ret = dlms_ber_decode_length(data + pos + 1, len - pos - 1, &oid_len, &lc);
        if (ret != DLMS_OK) return ret;
        pos += 1 + lc + oid_len;
    }

    /* Result */
    if (pos < len && data[pos] == 0x02) {
        pos++;
        if (pos < len) aare->result = data[pos++];
    }

    /* Result source diagnostics */
    if (pos < len && data[pos] == 0xA1) {
        pos++;
        uint32_t ds_len = 0;
        size_t lc = 0;
        ret = dlms_ber_decode_length(data + pos, len - pos, &ds_len, &lc);
        if (ret == DLMS_OK) {
            pos += lc + ds_len;
        }
    }

    /* Responding AE invocation id */
    if (pos < len && data[pos] == 0x80) {
        pos++;
        if (pos < len) pos += data[pos] + 1;
    }

    /* User information */
    if (pos < len && data[pos] == 0xBE) {
        uint32_t ui_len = 0;
        size_t lc = 0;
        ret = dlms_ber_decode_length(data + pos + 1, len - pos - 1, &ui_len, &lc);
        if (ret != DLMS_OK) goto done;
        pos += 1 + lc;
        size_t ui_end = pos + ui_len;

        while (pos < ui_end) {
            uint8_t tag = data[pos++];
            if (tag == 0x80) {
                if (pos >= ui_end) break;
                uint8_t alen = data[pos++];
                aare->auth_value = data + pos;
                aare->auth_value_len = alen;
                pos += alen;
            } else if (tag == 0x81) {
                /* Initiate response */
                uint32_t ir_len = 0;
                size_t lc2 = 0;
                ret = dlms_ber_decode_length(data + pos, ui_end - pos, &ir_len, &lc2);
                if (ret != DLMS_OK) break;
                pos += lc2;
                if (pos + ir_len > ui_end) break;
                dlms_initiate_response_t resp;
                size_t ic = 0;
                dlms_initiate_response_decode(data + pos, ir_len, &resp, &ic);
                aare->dlms_version = resp.dlms_version;
                aare->auth_mech = resp.auth_mech;
                aare->negotiated_max_pdu_size = resp.negotiated_max_pdu_size;
                aare->negotiated_conformance = resp.negotiated_conformance;
                if (resp.server_system_title_len > 0) {
                    memcpy(aare->server_ap_title, resp.server_system_title, resp.server_system_title_len);
                    aare->system_title = aare->server_ap_title;
                    aare->system_title_len = resp.server_system_title_len;
                }
                pos += ir_len;
            } else {
                break;
            }
        }
    }

done:
    *consumed = hc + header.length;
    return DLMS_OK;
}
