#include "contiki.h"
#include "sys/cc.h"
#include "vip-interface.h"
#include "vip-constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

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
        vip_parse_vrr(vip_pkt);
        type_handler->vrr_handler(vip_pkt);
        break;
    case VIP_TYPE_VRA:
        vip_parse_vra(vip_pkt);
        type_handler->vra_handler(vip_pkt);
        break;
    case VIP_TYPE_VRC:
        type_handler->vrc_handler(vip_pkt);
        break;
    case VIP_TYPE_REL:
        type_handler->rel_handler(vip_pkt);
        break;
    case VIP_TYPE_SER:
        vip_parse_ser(vip_pkt);
        type_handler->ser_handler(vip_pkt);
        break;
    case VIP_TYPE_SEA:
        vip_parse_sea(vip_pkt);
        type_handler->sea_handler(vip_pkt);
        break;
    case VIP_TYPE_SEC:
        vip_parse_sec(vip_pkt);
        type_handler->sec_handler(vip_pkt);
        break;
    case VIP_TYPE_VSD:
        vip_parse_vsd(vip_pkt);
        type_handler->vsd_handler(vip_pkt);
        break;
    case VIP_TYPE_ALLOC:
        if(vip_pkt->total_len > VIP_COMMON_HEADER_LEN) {
            vip_payload_test(vip_pkt);
        }
        type_handler->allocate_vt_handler(vip_pkt);
    } 
}