#include "contiki.h"

typedef struct vip_service_tuple vip_service_tuple_t;

extern process_event_t vg_rcv_event;
extern process_event_t vg_snd_event;

struct vip_service_tuple {
    vip_service_tuple_t *next;
    int service_index;
    char* service_id;
};

PROCESS_NAME(vg_process);