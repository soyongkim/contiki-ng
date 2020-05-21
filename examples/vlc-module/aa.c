
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aa.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "net/netstack.h"

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
extern coap_resource_t res_aa;
extern vip_entity_t aa_type_handler;

/* test event process */
process_event_t aa_rcv_event, aa_snd_event;

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];
static char query[11] = { "?src=" };

PROCESS(aa_process, "AA");
AUTOSTART_PROCESSES(&aa_process);

PROCESS_THREAD(aa_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  sprintf(query + 5, "%d", node_id);

  aa_rcv_event = process_alloc_event();
  aa_snd_event = process_alloc_event();
  
  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_aa, VIP_AA_URL);

  /* vip packet */
  vip_message_t *rcv_pkt, *snd_pkt;

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == aa_rcv_event) {
        rcv_pkt = (vip_message_t *)data;
        printf("Type[%d]\n", rcv_pkt->type);

        vip_route(rcv_pkt, &aa_type_handler);
      }
      else if(ev == aa_snd_event) {
        snd_pkt = (vip_message_t *)data;
        vip_request(snd_pkt);
      }
  }

  PROCESS_END();
}

static void
vip_request_callback(coap_callback_request_state_t *callback_state) {
}

static void
vip_request(vip_message_t *snd_pkt) {
  /* set vip endpoint */
  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(request, snd_pkt->dest_path);
  coap_set_header_uri_query(request, query);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  printf("T:%d | vr-id:%d\n", snd_pkt->type, snd_pkt->vr_id);

  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
