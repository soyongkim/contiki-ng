#include "contiki.h"
#include "sys/cc.h"
#include "lib/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "vip-interface.h"

void
vip_route(vip_message_t *vip_pkt, vip_entity_t *type_handler) {
    /* parse type field and payload */
    switch (vip_pkt->type)
    {
    case VIP_TYPE_BEACON:
        vip_parse_beacon(vip_pkt);
        type_handler->beacon_handler(vip_pkt);
        break;
    case VIP_TYPE_VRR:
        /* code */
        vip_parse_VRR(vip_pkt);
        type_handler->vrr_handler(vip_pkt);
        break;
    case VIP_TYPE_VRA:
        vip_parse_VRA(vip_pkt);
        type_handler->vra_handler(vip_pkt);
        break;
    case VIP_TYPE_VRC:
        vip_parse_VRC(vip_pkt);
        type_handler->vrc_handler(vip_pkt);
        break;
    case VIP_TYPE_REL:
        vip_parse_REL(vip_pkt);
        type_handler->rel_handler(vip_pkt);
        break;
    case VIP_TYPE_SER:
        vip_parse_SER(vip_pkt);
        type_handler->ser_handler(vip_pkt);
        break;
    case VIP_TYPE_SEA:
        vip_parse_SEA(vip_pkt);
        type_handler->sea_handler(vip_pkt);
        break;
    case VIP_TYPE_SEC:
        /* echo SEA */
        vip_parse_SEA(vip_pkt);
        type_handler->sec_handler(vip_pkt);
        break;
    case VIP_TYPE_SD:
        vip_parse_SD(vip_pkt);
        type_handler->sd_handler(vip_pkt);
        break;
    case VIP_TYPE_SDA:
        vip_parse_SDA(vip_pkt);
        type_handler->sda_handler(vip_pkt);
        break;
    case VIP_TYPE_VU:
        vip_parse_VU(vip_pkt);
        type_handler->vu_handler(vip_pkt);
        break;
    case VIP_TYPE_VM:
        vip_parse_VM(vip_pkt);
        type_handler->vu_handler(vip_pkt);
        break;
    case VIP_TYPE_ALLOW:
        if(vip_pkt->total_len > 8)
            vip_payload_test(vip_pkt);
        type_handler->allocate_vt_handler(vip_pkt);
    } 
}