#include "vip.h"

extern snd_buf;

typedef struct vip_snd_buf_s {
    vip_snd_buf_t* next;
    void* vip_pkt;
} vip_snd_buf_t;



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

#define TYPE_HANDLER(name, beacon_handler, vrr_handler, vra_handler, vrc_handler, rel_handler, ser_handler, sea_handler, sec_handler, sd_handler, sda_handler, allocate_vt_handler) \
vip_entity_t name = { beacon_handler, vrr_handler, vra_handler, vrc_handler, rel_handler, ser_handler, sea_handler, sec_handler, sd_handler, sda_handler, allocate_vt_handler };

void vip_route(vip_message_t *received_pkt, vip_entity_t *type_handler);

void vip_push_snd_buf(void* vip_pkt);
void* vip_front_snd_buf();
void vip_pop_snd_buf();