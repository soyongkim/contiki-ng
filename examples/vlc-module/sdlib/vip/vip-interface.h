#include "vip.h"
#include "coap-callback-api.h"

typedef struct vip_entity_s vip_entity_t;
typedef void (* vip_type_handler_t)(vip_message_t *received_pkt);

struct vip_entity_s {
    vip_type_handler_t beacon_handler;
    vip_type_handler_t vrr_handler;
    vip_type_handler_t vra_handler;
    vip_type_handler_t vrc_handler;
    vip_type_handler_t rel_handler;
    vip_type_handler_t ser_handler;
    vip_type_handler_t sea_handler;
    vip_type_handler_t sec_handler;
    vip_type_handler_t sd_handler;
    vip_type_handler_t sda_handler;
    vip_type_handler_t allocate_vt_handler;
};


typedef struct vip_transmit_s vip_transmit_t;
typedef void (* vip_request_t)(vip_message_t *send_pkt);
typedef void (* vip_req_callback_t)(coap_callback_request_state_t *callback_state);

struct vip_transmit_s {
    vip_request_t vip_request;
    vip_req_callback_t vip_callback_request;
};

#define TYPE_HANDLER(name, beacon_handler, vrr_handler, vra_handler, vrc_handler, rel_handler, ser_handler, sea_handler, sec_handler, sd_handler, sda_handler, allocate_vt_handler) \
vip_entity_t name = { beacon_handler, vrr_handler, vra_handler, vrc_handler, rel_handler, ser_handler, sea_handler, sec_handler, sd_handler, sda_handler, allocate_vt_handler };

#define VIP_TRANSMIT(name, vip_request, vip_callback_request) \
vip_transmit_t name = { vip_request, vip_callback_request};


void vip_route(vip_message_t *received_pkt, vip_entity_t *type_handler);