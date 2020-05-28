#ifndef AA
#define AA
#include "contiki.h"
#include "vip.h"
#include "vip-interface.h"
#include "vip-buf.h"
#include "vip-constants.h"

typedef struct se_cache_s se_cache_t;

struct se_cache_s {
    se_cache_t* next;
    int vr_id;
    int session_id;
    int vg_seq;
};



extern process_event_t aa_snd_event;

PROCESS_NAME(aa_process);

#endif