#ifndef VR
#define VR

#include "contiki.h"
#include "vip.h"
#include "vip-interface.h"
#include "vip-buf.h"
#include "vip-constants.h"

extern process_event_t vr_snd_event;

int rcv_nonce;

PROCESS_NAME(vr_process);
#endif