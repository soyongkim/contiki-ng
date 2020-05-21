
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

/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request(vip_message_t *snd_pkt);

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_vr;
extern vip_entity_t vr_type_handler;

/* test event process */
process_event_t vr_rcv_event, vr_snd_event;

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];
static char query[11] = { "?src=" };


static char test_addr[50];

PROCESS(vr_process, "VR");
AUTOSTART_PROCESSES(&vr_process);

PROCESS_THREAD(vr_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();
  sprintf(query + 5, "%d", node_id);

  make_coap_uri(test_addr, node_id);


  vr_rcv_event = process_alloc_event();
  vr_snd_event = process_alloc_event();

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_vr, "vip/vr");

  /* vip packet */
  vip_message_t *rcv_pkt, *snd_pkt;

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == vr_rcv_event) {
        rcv_pkt = (vip_message_t *)data;
        // 여기서 route를 실행해야함 aa 프로세스가 route해서 보내야함
        vip_route(rcv_pkt, &vr_type_handler);
      }
      else if(ev == vr_snd_event) {
        snd_pkt = (vip_message_t *)data;
        printf("Test: %s | %s\n", snd_pkt->dest_coap_addr, snd_pkt->src_coap_addr);
        vip_request(snd_pkt);
      }
  }

  PROCESS_END();
}

static void
vip_request_callback(coap_callback_request_state_t *callback_state) {
  coap_request_state_t *state = &callback_state->state;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
      printf("CODE:%d\n", state->response->code);
      if(state->response->code > 100) {
          printf("4.xx -> So.. try to retransmit\n");
          coap_send_request(callback_state, state->request->src_ep, state->request, vip_request_callback);
      }
  }
}

static void
vip_request(vip_message_t *snd_pkt) {
  /* set vip endpoint */
  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(request, snd_pkt->dest_path);
  //coap_set_header_uri_host(request, snd_pkt->src_coap_addr);
  coap_set_header_uri_query(request, query);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  printf("Send[%d] from %d\n", snd_pkt->type, node_id);
  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
