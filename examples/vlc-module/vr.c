
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vr.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "net/netstack.h"
#include "cooja_addr.h"

/* for ROOT in RPL */
#include "contiki-net.h"

/* Node ID */
#include "sys/node-id.h"


/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_vr;
extern vip_entity_t vr_type_handler;

/* test event process */
process_event_t vr_snd_event;

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];


/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request(vip_message_t *snd_pkt);



PROCESS(vr_process, "VR");
AUTOSTART_PROCESSES(&vr_process);

PROCESS_THREAD(vr_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  vr_snd_event = process_alloc_event();

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_vr, "vip/vr");

  /* vip packet */
  vip_message_t *snd_pkt;

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == vr_snd_event) {
        snd_pkt = (vip_message_t *)data;
        vip_request(snd_pkt);
      }
  }

  PROCESS_END();
}

static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  const char *nonce = NULL;
  coap_request_state_t *state = &res_callback_state->state;

  printf("callback - %d\n", state->status);

  if(state->status == COAP_REQUEST_STATUS_FINISHED) {
      printf("Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
      if(state->response->code < 100) {
        if(coap_get_query_variable(state->response, "nonce", &nonce)) {
          rcv_nonce = atoi(nonce);
          printf("Nonce:%d\n", rcv_nonce);
        }
      }
      res_vr.trigger();
  }
}

static void
vip_request(vip_message_t *snd_pkt) {
  /* set vip endpoint */
  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);

  if(snd_pkt->re_flag)
    coap_init_message(request, COAP_TYPE_NON, COAP_POST, 0);
  else
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);

  coap_set_header_uri_path(request, snd_pkt->dest_path);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  if(snd_pkt->query_len > 0)
    coap_set_header_uri_query(request, snd_pkt->query);

  printf("Send from %s to %s\n", snd_pkt->query, snd_pkt->dest_coap_addr);
  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
