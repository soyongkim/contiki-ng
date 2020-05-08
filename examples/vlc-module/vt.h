#include "contiki.h"

extern process_event_t vt_rcv_event, vt_snd_event;
extern int vt_id;

PROCESS_NAME(vt_process);