#include "vip.h"

typedef struct vip_entity_s vip_entity_t;


typedef void (* vip_type_handler_t)(vip_message_t *received_pkt, 
                                    vip_message_t *send_pkt, 
                                    uint8_t *buffer, 
                                    uint16_t buffer_size);

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
    vip_type_handler_t vu_handler;
    vip_type_handler_t vm_handler;
};


#define TYPE_HANDLER(name, beacon_handler, vrr_handler, vra_handler, vrc_handler, rel_handler, ser_handler, sea_handler, sec_handler, sd_handler, sda_handler, vu_handler, vm_handler) \
vip_entity_t name = { beacon_handler, vrr_handler, vra_handler, vrc_handler, rel_handler, ser_handler, sea_handler, sec_handler, sd_handler, sda_handler, vu_handler, vm_handler };

void vip_engine_init(void);