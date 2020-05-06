#include "vip-engine.h"

#include "contiki.h"
#include "sys/cc.h"
#include "lib/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "aa.h"

int 
vip_route(vip_message_t *vip_pkt, vip_entity_t *type_handler) {
   /* start from common header */
    uint8_t *offset = vip_pkt->buffer + VIP_COMMON_HEADER_LEN;

    /* parse type field and payload */
    switch (vip_pkt->type)
    {
    case VIP_TYPE_BEACON:
        vip_parse_beacon(vip_pkt);
        type_handler->beacon_handler(vip_pkt);
        break;
    case VIP_TYPE_VRR:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        break;
    case VIP_TYPE_VRA:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        offset += 4;
        uint32_t service_num = (vip_pkt->total_len - VIP_COMMON_HEADER_LEN) / 4 + 4;
        for (uint32_t current_position = 0; current_position < service_num; current_position++)
        {
            vip_pkt->service_id[current_position] = vip_parse_int_option(offset, 4);
            offset += 4;
        }
        break;
    case VIP_TYPE_VRC:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);

        break;
    case VIP_TYPE_REL:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        break;
    case VIP_TYPE_SER:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->service_id[0] = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->vr_seq_number = vip_parse_int_option(offset, 4);
        break;
    case VIP_TYPE_SEA:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->service_id[0] = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->vg_seq_number = vip_parse_int_option(offset, 4);
        break;
    case VIP_TYPE_SEC:
        /* code */
        break;
    case VIP_TYPE_SD:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->service_id[0] = vip_parse_int_option(offset , 4);
        offset += 4;
        vip_pkt->vg_seq_number = vip_parse_int_option(offset, 4);
        offset += 4;
        memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
        break;
    case VIP_TYPE_SDA:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->service_id[0] = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->vr_seq_number = vip_parse_int_option(offset, 4);
        offset += 4;
        memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
        break;
    case VIP_TYPE_VU:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        break;
    case VIP_TYPE_VM:
        /* code */
        vip_pkt->vr_id = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->service_id[0] = vip_parse_int_option(offset, 4);
        offset += 4;
        vip_pkt->vg_seq_number = vip_parse_int_option(offset, 4);
        offset += 4;
        memcpy(vip_pkt->payload, offset, vip_pkt->total_len - (VIP_COMMON_HEADER_LEN + 12));
        break;
    }

    return 0;
}