
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
process_event_t vr_rcv_event;


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

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_vr, "vip/vr");

  /* vip packet */
  vip_message_t *rcv_pkt;

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == vr_rcv_event) {
        rcv_pkt = (vip_message_t *)data;
        // 여기서 route를 실행해야함 aa 프로세스가 route해서 보내야함
        vip_route(rcv_pkt, &vr_type_handler);
      }
  }

  PROCESS_END();
}