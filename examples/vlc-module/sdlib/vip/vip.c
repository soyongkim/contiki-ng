#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sys/cc.h"

#include "vip.h"

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

int vip_serialize_message(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset;

    /* Initialize */
    vip_pkt->buffer = buffer;

    LOG_DBG("-Serializing VIP %u", vip_pkt->type);

    /* set common header fields */
    vip_pkt->buffer[0] = vip_pkt->type;
    vip_pkt->buffer[1] = (uint8_t)(vip_pkt->total_len >> 16);
    vip_pkt->buffer[2] = (uint8_t)(vip_pkt->total_len >> 8);
    vip_pkt->buffer[3] = (uint8_t)(vip_pkt->total_len);

    vip_pkt->buffer[4] = (uint8_t)(vip_pkt->aa_id >> 8);
    vip_pkt->buffer[5] = (uint8_t)(vip_pkt->aa_id);

    vip_pkt->buffer[6] = (uint8_t)(vip_pkt->vt_id >> 8);
    vip_pkt->buffer[7] = (uint8_t)(vip_pkt->vt_id);

    /* set Type Specific Fields */
    switch (vip_pkt->type)
    {
    case VIP_TYPE_BEACON:
        /* code */
        offset = vip_serialize_beacon(vip_pkt, buffer);
        break;
    case VIP_TYPE_VRR:
        /* code */
        offset = vip_serialize_VRR(vip_pkt, buffer);
        break;
    case VIP_TYPE_VRA:
        /* code */
        offset = vip_serialize_VRA(vip_pkt, buffer);
        break;
    case VIP_TYPE_VRC:
        /* code */
        offset = vip_serialize_VRC(vip_pkt, buffer);
        break;
    case VIP_TYPE_REL:
        /* code */
        offset = vip_serialize_REL(vip_pkt, buffer);
        break;
    case VIP_TYPE_SER:
        /* code */
        offset = vip_serialize_SER(vip_pkt, buffer);
        break;
    case VIP_TYPE_SEA:
        /* code */
        offset = vip_serialize_SEA(vip_pkt, buffer);
        break;
    case VIP_TYPE_SEC:
        /* code */
        offset = vip_serialize_SEC(vip_pkt, buffer);
        break;
    case VIP_TYPE_SD:
        /* code */
        offset = vip_serialize_SD(vip_pkt, buffer);
        break;
    case VIP_TYPE_SDA:
        /* code */
        offset = vip_serialize_SDA(vip_pkt, buffer);
        break;
    case VIP_TYPE_VU:
        /* code */
        offset = vip_serialize_VU(vip_pkt, buffer);
        break;
    case VIP_TYPE_VM:
        /* code */
        offset = vip_serialize_VM(vip_pkt, buffer);
        break;
    }

    memmove(offset, vip_pkt->payload, vip_pkt->payload_len);

    return 1;
}

uint8_t *
vip_serialize_beacon(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    return offset;
}

uint8_t *
vip_serialize_VRR(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return offset + index;
}

uint8_t *
vip_serialize_VRA(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    uint16_t current_position;
    for (current_position = 0; current_position < vip_pkt->service_num; current_position)
    {
        index = vip_int_serialize(index, VIP_SERVICE_NUM, offset, vip_pkt->service_id[current_position]);
        offset += index;
    }

    return offset;
}

uint8_t *
vip_serialize_VRC(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return offset + index;
}

uint8_t *
vip_serialize_REL(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return offset + index;
}

uint8_t *
vip_serialize_SER(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id[0]);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vr_seq_number);

    return offset + index;
}

uint8_t *
vip_serialize_SEA(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id[0]);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vg_seq_number);

    return offset + index;
}

/* 보류 */
uint8_t *
vip_serialize_SEC(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;
    return offset;
}

uint8_t *
vip_serialize_SD(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id[0]);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vg_seq_number);

    return offset + index;
}

uint8_t *
vip_serialize_SDA(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id[0]);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vr_seq_number);

    return offset + index;
}

uint8_t *
vip_serialize_VU(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);

    return offset + index;
}

uint8_t *
vip_serialize_VM(vip_message_t *vip_pkt, uint8_t *buffer)
{
    uint8_t *offset = buffer + VIP_COMMON_HEADER_LEN;
    unsigned int index = 0;

    index = vip_int_serialize(index, VIP_VR_ID_LEN, offset, vip_pkt->vr_id);
    offset += index;
    index = vip_int_serialize(index, VIP_SERVICE_ID_LEN, offset, vip_pkt->service_id[0]);
    offset += index;
    index = vip_int_serialize(index, VIP_SEQ_LEN, offset, vip_pkt->vg_seq_number);

    return offset + index;
}

int
vip_parse_message(vip_message_t *vip_pkt, uint8_t *data, uint32_t data_len)
{
    int index = -1;
    memset(vip_pkt, 0, sizeof(vip_message_t));

    vip_pkt->buffer = data;

    /* parse common header field */
    vip_pkt->type = vip_pkt->buffer[++index];
    vip_pkt->total_len = vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
    vip_pkt->aa_id = vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
    vip_pkt->vt_id = vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];

    /* parse type field and payload */
    switch (vip_pkt->type)
    {
    case VIP_TYPE_BEACON:
        int uplink_id_len = vip_pkt->total_len - VIP_COMMON_HEADER_LEN;
        memcpy(vip_pkt->uplink_id, vip_pkt->buffer[VIP_COMMON_HEADER_LEN], uplink_id_len);
        break;
    case VIP_TYPE_VRR:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        break;
    case VIP_TYPE_VRA:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        int service_num = (vip_pkt->total_len - VIP_COMMON_HEADER_LEN)/4 + 4;
        for(int current_position = 0; current_position < service_num; current_position++) {
            vip_pkt->service_id[current_position] = 
                vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        }
        break;
    case VIP_TYPE_VRC:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        break;
    case VIP_TYPE_REL:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        break;
    case VIP_TYPE_SER:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->service_id[0] = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->vr_seq_number = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        break;
    case VIP_TYPE_SEA:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->service_id[0] = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->vg_seq_number = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        break;
    case VIP_TYPE_SEC:
        /* code */
        break;
    case VIP_TYPE_SD:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->service_id[0] = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->vg_seq_number = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        memcpy(vip_pkt->payload, vip_pkt->buffer[++index], vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
        break;
    case VIP_TYPE_SDA:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->service_id[0] = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->vr_seq_number = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        memcpy(vip_pkt->payload, vip_pkt->buffer[++index], vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
        break;
    case VIP_TYPE_VU:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        break;
    case VIP_TYPE_VM:
        /* code */
        vip_pkt->vr_id = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->service_id[0] = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        vip_pkt->vg_seq_number = vip_pkt->buffer[++index] << 24 | vip_pkt->buffer[++index] << 16 | vip_pkt->buffer[++index] << 8 | vip_pkt->buffer[++index];
        memcpy(vip_pkt->payload, vip_pkt->buffer[++index], vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
        break;
    }

    return 0;
}