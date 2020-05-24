#include "contiki.h"

extern process_event_t vr_snd_event;

static int rcv_nonce;

PROCESS_NAME(vr_process);