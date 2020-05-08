#include "contiki.h"

extern process_event_t aa_rcv_event, aa_snd_event;

PROCESS_NAME(aa_process);

void my_coap_request(vip_message_t *vip_pkt);
void aa_coap_request_handler(vip_message_t *res);