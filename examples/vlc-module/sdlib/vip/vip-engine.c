#include "vip-engine.h"

#include "contiki.h"
#include "sys/cc.h"
#include "lib/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

PROCESS(vip_engine, "VIP Engine");

LIST(vip_type_handlers);
static uint8_t is_init = 0;

void
vip_add_handler(vip_type_handler_t *handler) {
    list_add(vip_type_handlers, handler);
}

void
vip_remove_handler(vip_type_handler_t *handler) {
    list_remove(vip_type_handlers, handler);
}

void
vip_engine_init(void)
{
  /* avoid initializing twice */
  if(is_init) {
    return;
  }
  is_init = 1;

  printf("Starting VIP engine...\n");

  list_init(vip_type_handlers);

  process_start(&vip_engine, NULL);
}

PROCESS_THREAD(vip_engine, ev, data)
{
  PROCESS_BEGIN();
  int count = 0;

  while(1) {
    //PROCESS_YIELD();
    
    printf("VIP Engine Test Count %d\n", count++);

    }

  PROCESS_END();
}