#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sys/cc.h"
#include "sdlib/configure_cooja/cooja_addr.h"
#include "vip.h"
#include "vip-constants.h"
#include "sys/node-id.h"


/* ----------------------------------------------------------------- vip pkt configure -----------------------------------------*/

void vip_init_message(vip_message_t *vip_pkt, uint8_t type,
                      uint16_t aa_id, uint16_t vt_id, uint32_t vr_id)
{
    memset(vip_pkt, 0, sizeof(vip_message_t));

    vip_pkt->type = type;
    vip_pkt->aa_id = aa_id;
    vip_pkt->vt_id = vt_id;
    vip_pkt->vr_id = vr_id;
}

void vip_clear_message(vip_message_t *vip_pkt)
{
    memset(vip_pkt, 0, sizeof(vip_message_t));
}

void vip_set_non_flag(vip_message_t* vip_pkt)
{
    /* 1 is COAP_TYPE_NON */
    vip_pkt->re_flag = 1;
}


/*  ------------------------------------------- Set Type header field -----------------------------------------------------*/

void vip_set_payload(vip_message_t* vip_pkt, void* payload, size_t payload_len)
{
    vip_pkt->payload = payload;
    vip_pkt->payload_len = payload_len;
}

void vip_set_field_beacon(vip_message_t* vip_pkt, char* uplink_id)
{
    vip_pkt->uplink_id = uplink_id;
}

void vip_set_field_vrr(vip_message_t *vip_pkt, int nonce)
{
    vip_pkt->nonce = nonce;
}

void vip_set_field_vra(vip_message_t *vip_pkt, int nonce)
{
    vip_pkt->nonce = nonce;
}

void vip_set_field_ser(vip_message_t *vip_pkt, int session_id, int vr_seq)
{
    vip_pkt->session_id = session_id;
    vip_pkt->vr_seq = vr_seq;
}

void vip_set_field_sea(vip_message_t *vip_pkt, int session_id, int vg_seq)
{
    vip_pkt->session_id = session_id;
    vip_pkt->vg_seq = vg_seq;
}

void vip_set_field_sec(vip_message_t *vip_pkt, int session_id, int vg_seq)
{
    vip_pkt->session_id = session_id;
    vip_pkt->vg_seq = vg_seq;
}

void vip_set_field_vsd(vip_message_t *vip_pkt, int session_id, int seq, void* payload, size_t payload_len)
{
    vip_pkt->session_id = session_id;
    vip_pkt->seq = seq;
    vip_set_payload(vip_pkt, payload, payload_len);
}

void vip_set_field_alloc(vip_message_t *vip_pkt, char* uplink_id)
{
    /* payload test */
    vip_set_payload(vip_pkt, (void *)uplink_id, strlen(uplink_id));
}

void vip_set_query(vip_message_t *vip_pkt, char *query)
{
    vip_pkt->query = query;
    vip_pkt->query_len = strlen(query);
}

void vip_set_dest_ep_cooja(vip_message_t *vip_pkt, char *dest_addr, int dest_node_id, char *dest_path)
{
    make_coap_uri(dest_addr, dest_node_id);
    vip_pkt->dest_coap_addr = dest_addr;
    vip_pkt->dest_path = dest_path;
}

int vip_int_serialize(unsigned int cur_offset, unsigned int space, uint8_t *buffer, uint32_t value)
{
    switch (space)
    {
    case 4:
        buffer[cur_offset++] = (uint8_t)(value >> 24);
    case 3:
        buffer[cur_offset++] = (uint8_t)(value >> 16);
    case 2:
        buffer[cur_offset++] = (uint8_t)(value >> 8);
    case 1:
        buffer[cur_offset++] = (uint8_t)(value);
    }
    return cur_offset;
}

void vip_memset_int(uint8_t *buffer, unsigned int space, uint32_t value)
{
    memset(buffer, value, space);
}

uint32_t
vip_parse_int_option(uint8_t *bytes, size_t length)
{
    uint32_t var = 0;
    int i = 0;

    while (i < length)
    {
        var <<= 8;
        var |= bytes[i++];
    }
    return var;
}


/* ----------------------------------------------- vip pkt serialize ---------------------------------------------------*/
static void
vip_serialize_array(uint8_t *buffer, uint8_t *array, size_t length)
{
    memcpy(buffer, array, length);
}

int vip_serialize_beacon(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_serialize_array(offset, (uint8_t *)(vip_pkt->uplink_id), strlen(vip_pkt->uplink_id));
    return strlen(vip_pkt->uplink_id);
}

int vip_serialize_vrr(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;
    index += vip_int_serialize(index, 4, offset, vip_pkt->nonce);

    printf("[vip] Serialize VR-ID:%d | Nonce:%d\n", vip_pkt->vr_id, vip_pkt->nonce);
    return index;
}

int vip_serialize_vra(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;
    index += vip_int_serialize(index, 4, offset, vip_pkt->nonce);

    return index;
}

int vip_serialize_vrc(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index += vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return index;
}

int vip_serialize_rel(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index += vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return index;
}

int vip_serialize_ser(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index += vip_int_serialize(index, 4, offset, vip_pkt->session_id);
    index += vip_int_serialize(index, 4, offset, vip_pkt->vr_seq);

    printf("[vip] Serialize Session-ID:%x | VR-SEQ:%d\n", vip_pkt->session_id, vip_pkt->vr_seq);

    return index;
}

int vip_serialize_sea(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index += vip_int_serialize(index, 4, offset, vip_pkt->session_id);
    index += vip_int_serialize(index, 4, offset, vip_pkt->vg_seq);

    return index;
}

int vip_serialize_sec(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index += vip_int_serialize(index, 4, offset, vip_pkt->session_id);
    index += vip_int_serialize(index, 4, offset, vip_pkt->vg_seq);

    return index;
}

int vip_serialize_vsd(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index += vip_int_serialize(index, 4, offset, vip_pkt->session_id);
    index += vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->seq);

    return index;
}

int vip_serialize_message(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset;
    int total_len = 0;

    /* Initialize */
    vip_pkt->buffer = buffer;
    offset = vip_pkt->buffer;

    /* set common header fields without total len*/
    vip_pkt->buffer[0] = vip_pkt->type;

    vip_pkt->buffer[4] = (uint8_t)(vip_pkt->aa_id >> 8);
    vip_pkt->buffer[5] = (uint8_t)(vip_pkt->aa_id);

    vip_pkt->buffer[6] = (uint8_t)(vip_pkt->vt_id >> 8);
    vip_pkt->buffer[7] = (uint8_t)(vip_pkt->vt_id);

    vip_pkt->buffer[8] = (uint8_t)(vip_pkt->vr_id >> 24);
    vip_pkt->buffer[9] = (uint8_t)(vip_pkt->vr_id >> 16);
    vip_pkt->buffer[10] = (uint8_t)(vip_pkt->vr_id >> 8);
    vip_pkt->buffer[11] = (uint8_t)(vip_pkt->vr_id);

    total_len = VIP_COMMON_HEADER_LEN;

    // total_len += vip_int_serialize(total_len, 1, offset, vip_pkt->type);

    // /* for total_length field */
    // total_len += 3;

    // total_len += vip_int_serialize(total_len, 2, offset, vip_pkt->aa_id);
    // total_len += vip_int_serialize(total_len, 2, offset, vip_pkt->vt_id);
    // total_len += vip_int_serialize(total_len, 4, offset, vip_pkt->vr_id);

    /* set Type Specific Fields */
    switch (vip_pkt->type)
    {
    case VIP_TYPE_BEACON:
        /* code */
        total_len += vip_serialize_beacon(vip_pkt);
        break;
    case VIP_TYPE_VRR:
        total_len += vip_serialize_vrr(vip_pkt);
        break;
    case VIP_TYPE_VRA:
        /* code */
        total_len += vip_serialize_vra(vip_pkt);
        break;
    case VIP_TYPE_SER:
        /* code */
        total_len += vip_serialize_ser(vip_pkt);
        break;
    case VIP_TYPE_SEA:
        /* code */
        total_len += vip_serialize_sea(vip_pkt);
        break;
    case VIP_TYPE_SEC:
        /* code */
        total_len += vip_serialize_sec(vip_pkt);
        break;
    case VIP_TYPE_VSD:
        /* code */
        total_len += vip_serialize_vsd(vip_pkt);
        break;
    }

    /* serialize paylaod */
    offset = vip_pkt->buffer + total_len;
    if (vip_pkt->payload_len)
    {
        memmove(offset, vip_pkt->payload, vip_pkt->payload_len);
        total_len += vip_pkt->payload_len;
    }

    /* serialize total length */
    vip_pkt->total_len = total_len;
    vip_pkt->buffer[1] = (uint8_t)(vip_pkt->total_len >> 16);
    vip_pkt->buffer[2] = (uint8_t)(vip_pkt->total_len >> 8);
    vip_pkt->buffer[3] = (uint8_t)(vip_pkt->total_len);

    return total_len;
}


/*------------------------------------------------------ Data Parsing -----------------------------------------------------------*/

static uint8_t parse_payload[VIP_MAX_PKT_SIZE];

int vip_parse_common_header(vip_message_t *vip_pkt, uint8_t *data, uint16_t data_len)
{
    memset(vip_pkt, 0, sizeof(vip_message_t));

    vip_pkt->buffer = data;
    uint8_t *offset = data;

    /* parse common header field */
    vip_pkt->type = vip_parse_int_option(offset, 1);
    offset += 1;
    vip_pkt->total_len = vip_parse_int_option(offset, 3);
    offset += 3;

    if (vip_pkt->total_len != data_len)
        return VIP_ERROR;

    vip_pkt->aa_id = vip_parse_int_option(offset, 2);
    offset += 2;
    vip_pkt->vt_id = vip_parse_int_option(offset, 2);
    offset += 2;
    vip_pkt->vr_id = vip_parse_int_option(offset, 4);

    return VIP_NO_ERROR;
}

void vip_parse_beacon(vip_message_t *vip_pkt)
{
    /* Start from common header's end */
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    /* parsing uplink_id */
    vip_pkt->uplink_id = (char *)malloc((vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    memcpy(vip_pkt->uplink_id, (char *)offset, (vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    vip_pkt->uplink_id[(vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1] = '\0';
}

void vip_parse_vrr(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->nonce = vip_parse_int_option(offset, 4);
}

void vip_parse_vra(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->nonce = vip_parse_int_option(offset, 4);
}

void vip_parse_vrc(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->nonce = vip_parse_int_option(offset, 4);
}

void vip_parse_rel(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->nonce = vip_parse_int_option(offset, 4);
}

void vip_parse_ser(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->session_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vr_seq = vip_parse_int_option(offset, 4);
    printf("[vip] Parse Session-ID:%x VR-ID:%d\n", vip_pkt->session_id, vip_pkt->vr_seq);
}

void vip_parse_sea(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->session_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vg_seq = vip_parse_int_option(offset, 4);
}

void vip_parse_sec(vip_message_t* vip_pkt)
{
    /* same field */
    vip_parse_sea(vip_pkt);
}

void vip_parse_vsd(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->session_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->seq = vip_parse_int_option(offset, 4);
    offset += 4;

    //printf("close payload\n");
    vip_pkt->payload = (void *)parse_payload;
    memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 8));
}

void vip_payload_test(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->uplink_id = (char *)malloc((vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    memcpy(vip_pkt->uplink_id, (char *)offset, (vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    vip_pkt->uplink_id[(vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1] = '\0';
}





/*------------------------------------------------------ coap query handle -----------------------------------------------------------*/
void vip_init_query(vip_message_t *vip_pkt, char *query) {
    vip_pkt->query_len = 0;
    memset(query, 0, sizeof(char)*VIP_MAX_QUERY_SIZE);
}


void vip_make_query_src(char *query, int query_len, int src_id)
{
    if (!query_len)
    {
        strcpy(query, "?src=");
    }
    else
    {
        strcat(query, "&src=");
    }

    char tochar[5];
    sprintf(tochar, "%d", src_id);
    strcat(query, tochar);
}

void vip_make_query_nonce(char *query, int query_len, int value)
{
    if (!query_len)
    {
        strcpy(query, "?nonce=");
    }
    else
    {
        strcat(query, "&nonce=");
    }

    char tochar[5];
    sprintf(tochar, "%d", value);
    strcat(query, tochar);
}

void vip_make_query_timer(char* query, int query_len, int flag)
{
    if (!query_len)
    {
        strcpy(query, "?timer=");
    }
    else
    {
        strcat(query, "&timer=");
    }

    char tochar[5];
    sprintf(tochar, "%d", flag);
    strcat(query, tochar);
}

void vip_make_query_goal(char* query, int query_len, int flag)
{
    if (!query_len)
    {
        strcpy(query, "?goal=");
    }
    else
    {
        strcat(query, "&goal=");
    }

    char tochar[5];
    sprintf(tochar, "%d", flag);
    strcat(query, tochar);
}

void vip_make_query_start_time(char *query, int query_len, uint32_t start_time)
{
    if (!query_len)
    {
        strcpy(query, "?start=");
    }
    else
    {
        strcat(query, "&start=");
    }

    char tochar[5];
    sprintf(tochar, "%ld", start_time);
    strcat(query, tochar);
}

void vip_make_query_transmit_time(char *query, int query_len, uint32_t transmit_time)
{
    if (!query_len)
    {
        strcpy(query, "?transmit=");
    }
    else
    {
        strcat(query, "&transmit=");
    }

    char tochar[5];
    sprintf(tochar, "%ld", transmit_time);
    strcat(query, tochar);
}
