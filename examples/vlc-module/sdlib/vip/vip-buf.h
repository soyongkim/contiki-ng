#include "vip.h"

typedef struct vip_snd_buf_s vip_snd_buf_t;
typedef struct vip_snd_node_s vip_snd_node_t;


struct vip_snd_buf_s {
    vip_snd_node_t* head;
    vip_snd_node_t* tail;
};


struct vip_snd_node_s {
    vip_snd_node_t* next;
    vip_message_t* vip_pkt;
};

void vip_push_snd_buf(vip_message_t* vip_pkt);
vip_message_t* vip_front_snd_buf();
void vip_pop_snd_buf();