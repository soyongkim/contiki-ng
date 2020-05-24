#include "contiki.h"

#ifndef VR
#define VR
extern process_event_t vr_snd_event;

static int rcv_nonce;

PROCESS_NAME(vr_process);
#endif