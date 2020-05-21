#include "contiki.h"

extern process_event_t aa_rcv_event;
static char query[11];

PROCESS_NAME(aa_process);