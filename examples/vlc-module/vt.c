
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vt.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "net/netstack.h"
#include "sys/ctimer.h"
#include "random.h"

/* Node ID */
#include "sys/node-id.h"

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_vt;
extern vip_entity_t vt_type_handler;


/* test event process */
process_event_t vt_snd_event;

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];  

/* vip packet */
static  vip_message_t* snd_pkt;

static struct etimer et;
//static struct ctimer ct;


/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request();


static void timer_callback(void* data);
static void init();


PROCESS(vt_process, "VT");
AUTOSTART_PROCESSES(&vt_process);

PROCESS_THREAD(vt_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  vt_snd_event = process_alloc_event();
  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_vt, VIP_VT_URL);

  etimer_set(&et, CLOCK_SECOND);
  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

     if(ev == vt_snd_event) {
        vip_push_snd_buf((vip_message_t*)data);
        init();
      }

      if(etimer_expired(&et))
      {
        timer_callback(data);
      }
  }

  PROCESS_END();
}

static void
timer_callback(void* data)
{
  printf("SEND!\n");
  vip_request();
}

static void init()
{
  // for a lot of vr simul
  //int random_incount = rand() % 500 + 300;
  int random_incount = rand() % 500 + 300;
  printf("Set Send Timer %d\n", random_incount);

  //ctimer_set(&ct, random_incount, timer_callback, NULL);
  etimer_reset(&et);
}



static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  coap_request_state_t *state = &res_callback_state->state;
  /* Process ack-pkt from vg */
  if (state->status == COAP_REQUEST_STATUS_RESPONSE)
  {
    printf("Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
  }
}

static void
vip_request() {
  /* set vip endpoint */
  while(!vip_is_empty())
  {
    snd_pkt = vip_front_snd_buf();

    printf("type: %d\n", snd_pkt->type);
    if(snd_pkt->type == VIP_TYPE_VSD)
    {
      int loss_simul_var = random_rand() % 100;
      if (loss_simul_var >= 0)
      {
        printf("SUCCESS!\n");
      }
      else
      {
        printf("LOSS! -----------------------------------------------------------------------\n");
        vip_pop_snd_buf();
        break;
      }
    }

    if (snd_pkt->query_len)
    {
      snd_pkt->start_time = RTIMER_NOW()/1000;
      vip_make_query_start_time(snd_pkt->query, snd_pkt->query_len, (uint32_t)snd_pkt->start_time);
      printf("time check! %d\n", snd_pkt->start_time);
      printf("Query:%s\n", snd_pkt->query);
    }

    coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);

    coap_init_message(request, COAP_TYPE_NON, COAP_POST, 0);
    coap_set_header_uri_path(request, snd_pkt->dest_path);
    coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

    if(snd_pkt->query_len)
      coap_set_header_uri_query(request, snd_pkt->query);


    printf("Send to %s - clock_time(%d)\n", snd_pkt->dest_coap_addr, clock_time());
    coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);

    vip_pop_snd_buf();
  }

  etimer_pending();
}
