#include "contiki.h"
#include "vip.h"
#include "vip-interface.h"
#include "vip-buf.h"
#include "vip-constants.h"


extern process_event_t vt_rcv_event;
extern process_event_t vt_snd_event;

PROCESS_NAME(vt_process);