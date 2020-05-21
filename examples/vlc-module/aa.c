
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

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_aa;
extern vip_entity_t aa_type_handler;

/* test event process */
process_event_t aa_rcv_event, aa_snd_event;

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
      }
  }

  PROCESS_END();
}