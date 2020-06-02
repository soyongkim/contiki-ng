
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vg.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "net/netstack.h"


/* for ROOT in RPL */
#include "contiki-net.h"

/* Node ID */
#include "sys/node-id.h"

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_vg;
extern vip_entity_t vg_type_handler;


PROCESS(vg_process, "VG");
AUTOSTART_PROCESSES(&vg_process);

PROCESS_THREAD(vg_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  /* you must make this node first on cooja. so set the node id to 1 */
  printf("Node ID is %d\n", node_id);

  NETSTACK_ROUTING.root_start();
  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_vg, VIP_VG_URL);

  /* init vg service list */
  res_vg.trigger();

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}