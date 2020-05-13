
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aa.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "net/netstack.h"

/* Node ID */
#include "sys/node-id.h"

static void aa_coap_request_callback(coap_callback_request_state_t *callback_state);


/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_hello, res_aa;
extern vip_entity_t aa_type_handler;

/* test event process */
process_event_t aa_rcv_event, aa_snd_event;

/* vip packet */
vip_message_t *rcv_pkt, *snd_pkt;

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];


PROCESS(aa_process, "AA");
AUTOSTART_PROCESSES(&aa_process);

PROCESS_THREAD(aa_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  printf("Node ID is %d\n", node_id);

  aa_rcv_event = process_alloc_event();
  aa_snd_event = process_alloc_event();

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_aa, "vip/aa");
  coap_activate_resource(&res_hello, "test/hello");

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == aa_rcv_event) {
        rcv_pkt = (vip_message_t *)data;
        printf("type is %d\n", rcv_pkt->type);

        // 여기서 route를 실행해야함 aa 프로세스가 route해서 보내야함
        vip_route(rcv_pkt, &aa_type_handler);
      }
      else if(ev == aa_snd_event) {
        snd_pkt = (vip_message_t *)data;

        coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, snd_pkt->dest_url);
        coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

        printf("-- AA Send coap vip[%d] packet --\n", snd_pkt->type);

        coap_send_request(&callback_state, &dest_ep, request, aa_coap_request_callback);
      }

      printf("EVENT!\n");
  }

  PROCESS_END();
}

static void
aa_coap_request_callback(coap_callback_request_state_t *callback_state) {
  printf("AA CoAP Response Handler\n");
  //printf("CODE:%d\n", callback_state->state.response->code);
}
 
