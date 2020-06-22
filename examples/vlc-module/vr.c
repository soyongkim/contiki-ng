
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "vr.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "net/netstack.h"
#include "random.h"


#include "sys/ctimer.h"
#include "sys/rtimer.h"

/* for ROOT in RPL */
#include "contiki-net.h"

/* Node ID */
#include "sys/node-id.h"


/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_vr;
//extern coap_resource_t res_vr_stop_and_wait;
extern vip_entity_t vr_type_handler;

/* test event process */
process_event_t vr_snd_event;

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];

/* vip packet */
static  vip_message_t *snd_pkt;

static struct etimer et;
//static struct ctimer ct;
int random_incount;
/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request();


static void timer_callback(void* data);
static void init();

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
  //coap_activate_resource(&res_vr, "vip/vr");
  coap_activate_resource(&res_vr, VIP_VR_URL);
  
  etimer_set(&et, VIP_SEND_JITTER);

  /* Define application-specific events here. */
  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == vr_snd_event)
    {
      //vip_push_snd_buf((vip_message_t *)data);
      init();
    }

    if (etimer_expired(&et))
    {
      timer_callback(data);
    }
  }

  PROCESS_END();
}

static void
timer_callback(void* data)
{
  printf("SEND! - time:%d\n", clock_time());
  vip_request();
}

static void init()
{
  //ctimer_set(&ct, CLOCK_SECOND/100, timer_callback, NULL);
  etimer_restart(&et);
}

static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  const char *nonce = NULL;
  const char *timer = NULL;
  coap_request_state_t *state = &res_callback_state->state;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
      printf("Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
      printf("Same tick? => clock_time(%d)\n", clock_time());
      if(state->response->code < 100) {
        if(coap_get_query_variable(state->response, "nonce", &nonce)) {
          rcv_nonce = atoi(nonce);
          printf("Nonce:%d\n", atoi(nonce));
        }

        if(coap_get_query_variable(state->response, "timer", &timer)) {
            //res_vr.trigger();
        }
      }
  }
}

static void
vip_request() {
  while(!vip_is_empty())
  {
    snd_pkt = vip_front_snd_buf();

    /* set vip endpoint */
    coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);

    coap_init_message(request, snd_pkt->re_flag, COAP_POST, 0);

    coap_set_header_uri_path(request, snd_pkt->dest_path);
    coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

    if(snd_pkt->query_len)
      coap_set_header_uri_query(request, snd_pkt->query);

    printf("Send to %s - clock_time(%d)\n", snd_pkt->dest_coap_addr, clock_time());
    coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);

    vip_pop_snd_buf();
  }

  etimer_pending();
  printf("etimer_next_expireation_time: %d\n", etimer_next_expiration_time());
}