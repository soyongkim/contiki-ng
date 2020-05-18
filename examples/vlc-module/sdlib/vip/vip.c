

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sys/cc.h"
#include "coap-blocking-api.h"

#include "vip.h"

#include "sys/node-id.h"

/* Serialize Type Specific Header */
int vip_serialize_beacon(vip_message_t *message);
int vip_serialize_VRR(vip_message_t *message);
int vip_serialize_VRA(vip_message_t *message);
int vip_serialize_VRC(vip_message_t *message);
int vip_serialize_REL(vip_message_t *message);
int vip_serialize_SER(vip_message_t *message);
int vip_serialize_SEA(vip_message_t *message);
int vip_serialize_SEC(vip_message_t *message);
int vip_serialize_SD(vip_message_t *message);
int vip_serialize_SDA(vip_message_t *message);
int vip_serialize_VU(vip_message_t *message);
int vip_serialize_VM(vip_message_t *message);
int serialize_no_type(vip_message_t *message);

void vip_init_message(vip_message_t *message, uint8_t type,
                      uint16_t aa_id, uint16_t vt_id)
{
    memset(message, 0, sizeof(vip_message_t));

    message->type = type;
    message->aa_id = aa_id;
    message->vt_id = vt_id;
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

int 
vip_memset_int(uint8_t *buffer, unsigned int space, uint32_t value) {
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

int vip_serialize_message(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset;
    int total_len;

    /* Initialize */
    vip_pkt->buffer = buffer;

    /* set common header fields without total len*/
    vip_pkt->buffer[0] = vip_pkt->type;

    vip_pkt->buffer[4] = (uint8_t)(vip_pkt->aa_id >> 8);
    vip_pkt->buffer[5] = (uint8_t)(vip_pkt->aa_id);

    vip_pkt->buffer[6] = (uint8_t)(vip_pkt->vt_id >> 8);
    vip_pkt->buffer[7] = (uint8_t)(vip_pkt->vt_id);

    total_len = VIP_COMMON_HEADER_LEN;

    /* set Type Specific Fields */
    switch (vip_pkt->type)
    {
    case VIP_TYPE_BEACON:
        /* code */
        total_len += vip_serialize_beacon(vip_pkt);
        break;
    case VIP_TYPE_VRR:
        /* code */
        total_len += vip_serialize_VRR(vip_pkt);
        break;
    case VIP_TYPE_VRA:
        /* code */
        total_len += vip_serialize_VRA(vip_pkt);
        break;
    case VIP_TYPE_VRC:
        /* code */
        total_len += vip_serialize_VRC(vip_pkt);
        break;
    case VIP_TYPE_REL:
        /* code */
        total_len += vip_serialize_REL(vip_pkt);
        break;
    case VIP_TYPE_SER:
        /* code */
        total_len += vip_serialize_SER(vip_pkt);
        break;
    case VIP_TYPE_SEA:
        /* code */
        total_len += vip_serialize_SEA(vip_pkt);
        break;
    case VIP_TYPE_SEC:
        /* code */
        total_len += vip_serialize_SEC(vip_pkt);
        break;
    case VIP_TYPE_SD:
        /* code */
        total_len += vip_serialize_SD(vip_pkt);
        break;
    case VIP_TYPE_SDA:
        /* code */
        total_len += vip_serialize_SDA(vip_pkt);
        break;
    case VIP_TYPE_VU:
        /* code */
        total_len += vip_serialize_VU(vip_pkt);
        break;
    case VIP_TYPE_VM:
        /* code */
        total_len += vip_serialize_VM(vip_pkt);
        break;
    }

    /* serialize paylaod */
    offset = vip_pkt->buffer + total_len;
    if(vip_pkt->payload_len) {
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


static void
vip_serialize_array(uint8_t *buffer, uint8_t *array, size_t length) {
    memcpy(buffer, array, length);
}


int
vip_serialize_beacon(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_serialize_array(offset, (uint8_t *)(vip_pkt->uplink_id), strlen(vip_pkt->uplink_id));
    return strlen(vip_pkt->uplink_id);
}

int
vip_serialize_VRR(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return index;
}

int
vip_serialize_VRA(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    // unsigned int index = 0;
    // index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    // offset += index;
    
    vip_memset_int(offset, 4, vip_pkt->vr_id);



    // uint16_t current_position;
    // for (current_position = 0; current_position < vip_pkt->service_num; current_position++)
    // {
    //     memcpy(offset, vip_pkt->service_list[current_position], 15);
    //     offset += 15;
    // }

    //return (vip_pkt->service_num)*15;
    return 4;
}

int
vip_serialize_VRC(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return index;
}

int
vip_serialize_REL(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return index;
}

int
vip_serialize_SER(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vr_seq_number);

    return 12;
}

int
vip_serialize_SEA(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vg_seq_number);

    return 12;
}

/* 보류 */
int
vip_serialize_SEC(vip_message_t *vip_pkt)
{
    return 0;
}

int
vip_serialize_SD(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vg_seq_number);

    return 12;
}

int
vip_serialize_SDA(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vr_seq_number);

    return 12;
}

int
vip_serialize_VU(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return 4;
}

int
vip_serialize_VM(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vg_seq_number);

    return 12;
}



/* Data Parsing */
int 
vip_parse_common_header(vip_message_t *vip_pkt, uint8_t *data, uint16_t data_len)
{
    memset(vip_pkt, 0, sizeof(vip_message_t));

    vip_pkt->buffer = data;
    uint8_t *offset = data;

    /* parse common header field */
    vip_pkt->type = vip_parse_int_option(offset, 1);
    offset += 1;
    vip_pkt->total_len = vip_parse_int_option(offset, 3);
    offset += 3;


    if(vip_pkt->total_len != data_len)
        return VIP_ERROR;


    vip_pkt->aa_id = vip_parse_int_option(offset, 2);
    offset += 2;
    vip_pkt->vt_id = vip_parse_int_option(offset, 2);
    offset += 2;

    return VIP_NO_ERROR;
}

void
vip_parse_beacon(vip_message_t *vip_pkt)
{
    /* start from common header */
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    /* parsing uplink_id */
    vip_pkt->uplink_id = (char *)malloc((vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    memcpy(vip_pkt->uplink_id, (char *)offset, (vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    vip_pkt->uplink_id[(vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1] = '\0';
}

void 
vip_parse_VRR(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
}

void 
vip_parse_VRA(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
    offset += 4;
    printf("parse vra:%d\n", vip_pkt->vr_id);

    // /* parsing service */
    // for (uint32_t current_position = 0; current_position < vip_pkt->service_num; current_position++) {
    //     /* 꼼수 : 일단 미리 넣어둔 id만큼 15바이트씩만 읽도록 하자 */
    //     vip_pkt->service_list[current_position] = malloc(sizeof(char)*15);
    //     memset(vip_pkt->service_list[current_position], 0, 15);
    //     memcpy(vip_pkt->service_list[current_position], offset, 15);
    //     offset += 15;
    // }
}

void 
vip_parse_VRC(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
}

void 
vip_parse_REL(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
}

void 
vip_parse_SER(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);

    offset += 4;
    vip_pkt->service_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vr_seq_number = vip_parse_int_option(offset, 4);
}

void 
vip_parse_SEA(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->service_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vg_seq_number = vip_parse_int_option(offset, 4);
}

void 
vip_parse_SD(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->service_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vg_seq_number = vip_parse_int_option(offset, 4);
    offset += 4;
    memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
}

void 
vip_parse_SDA(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->service_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vr_seq_number = vip_parse_int_option(offset, 4);
    offset += 4;
    memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
}

void 
vip_parse_VU(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
}

void 
vip_parse_VM(vip_message_t *vip_pkt)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    vip_pkt->vr_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->service_id = vip_parse_int_option(offset, 4);
    offset += 4;
    vip_pkt->vg_seq_number = vip_parse_int_option(offset, 4);
    offset += 4;
    memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
}

void
vip_payload_test(vip_message_t *vip_pkt) {
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    vip_pkt->uplink_id = (char *)malloc((vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    memcpy(vip_pkt->uplink_id, (char *)offset, (vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1);
    vip_pkt->uplink_id[(vip_pkt->total_len) - VIP_COMMON_HEADER_LEN + 1] = '\0';
}

/* Data Configure */
int
vip_set_header_total_len(vip_message_t *vip_pkt, uint32_t total_len) {
    vip_pkt->total_len = total_len;
    return 1;
}


int 
vip_get_type_header_uplink_id(vip_message_t *vip_pkt, char **uplink_id) {
    *uplink_id = vip_pkt->uplink_id;
    return 1;
}


int
vip_set_type_header_uplink_id(vip_message_t *vip_pkt, char *uplink_id) {
    vip_pkt->uplink_id = uplink_id;
    return 1;
}


int 
vip_set_type_header_vr_id(vip_message_t *vip_pkt, uint32_t vr_id) {
    vip_pkt->vr_id = vr_id;
    return 1;
}

int 
vip_set_dest_ep(vip_message_t *vip_pkt, char *dest_addr, char *dest_url) {
    vip_pkt->dest_coap_addr = dest_addr;
    vip_pkt->dest_url = dest_url;
    return 1;
}

int 
vip_set_payload(vip_message_t *vip_pkt, void *payload, size_t payload_len) {
    vip_pkt->payload = payload;
    vip_pkt->payload_len = payload_len;
    return 1;
}


int
vip_set_service_list(vip_message_t *vip_pkt, char **service_list, size_t service_num) {
    vip_pkt->service_list = service_list;
    vip_pkt->service_num = service_num;
    return 1;
}