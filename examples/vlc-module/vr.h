#include "contiki.h"

extern process_event_t vr_rcv_event;
extern process_event_t vr_snd_event;
extern process_event_t vr_get_event;

PROCESS_NAME(vr_process);