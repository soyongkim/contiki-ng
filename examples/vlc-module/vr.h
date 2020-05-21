#include "contiki.h"

extern process_event_t vr_rcv_event;
extern process_event_t vr_snd_event;
static char query[11];

PROCESS_NAME(vr_process);