#include "contiki.h"
#include "coap-callback-api.h"

extern process_event_t aa_rcv_event;
extern process_event_t aa_snd_event;

/* for send packet */
coap_callback_request_state_t callback_state;
coap_endpoint_t dest_ep;
coap_message_t request[1];
char query[11] = { "?src=" };

/* using coap callback api */
void vip_request_callback(coap_callback_request_state_t *callback_state);
void vip_request(vip_message_t *snd_pkt);

PROCESS_NAME(aa_process);