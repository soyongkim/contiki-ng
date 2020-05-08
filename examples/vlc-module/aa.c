
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aa.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "vip-interface.h"
#include "net/netstack.h"

/* Node ID */
#include "sys/node-id.h"
/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_aaa;
extern vip_entity_t aa_type_handler;


/* test event process */
process_event_t aa_rcv_event, aa_snd_event;

/* vip packet */
vip_message_t *rcv_pkt, *snd_pkt;

PROCESS(aa_process, "AA");
AUTOSTART_PROCESSES(&aa_process);


static void
aa_coap_request_handler(coap_message_t *res) {
  const uint8_t *chunk;

  if(res == NULL) {
    puts("Request timed out");
    return;
  }

  coap_get_payload(res, &chunk);

  printf("Req-ack: %s\n", (char*)chunk);
}

static void
my_coap_request(vip_message_t *snd_pkt) {
  static coap_endpoint_t dest_ep;
  static coap_message_t request[1];

  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_host(request, snd_pkt->dest_url);

  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  printf("-- AA Send coap vip[%d] packet --\n", snd_pkt->type);
  /* 일단 확실히 전송이 되는것부터 테스트 */
  COAP_BLOCKING_REQUEST(&dest_ep, request, aa_coap_request_handler);
}




PROCESS_THREAD(aa_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  printf("Node ID is %d\n", node_id);
  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_aaa, "vip/aa");

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == aa_rcv_event) {
        res_aaa.trigger();

        rcv_pkt = (vip_message_t *)data;
        printf("type is %d\n", rcv_pkt->type);

        // 여기서 route를 실행해야함 aa 프로세스가 route해서 보내야함
        vip_route(rcv_pkt, &aa_type_handler);
      }
      else if(ev == aa_snd_event) {
        snd_pkt = (vip_message_t *)data;
        my_coap_request(snd_pkt);
      }

      printf("EVENT!\n");
  }

  PROCESS_END();
}

