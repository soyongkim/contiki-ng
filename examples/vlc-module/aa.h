#include "contiki.h"

extern process_event_t aa_rcv_event;
extern process_event_t aa_snd_event;


/* using coap callback api */
void vip_request_callback(coap_callback_request_state_t *callback_state);
void vip_request(vip_message_t *snd_pkt);


PROCESS_NAME(aa_process);