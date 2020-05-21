#include "contiki.h"

extern process_event_t aa_rcv_event;
extern process_event_t aa_snd_event;

char query[11] = { "?src=" };

PROCESS_NAME(aa_process);