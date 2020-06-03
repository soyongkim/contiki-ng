
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aa.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "net/netstack.h"
#include "sys/ctimer.h"
#include "sys/rtimer.h"
#include "sys/cooja_mt.h"


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
process_event_t aa_snd_event;


/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];

/* vip packet */
static  vip_message_t* snd_pkt;
static struct ctimer ct;


/* test multi thead */
static struct cooja_mtarch_thread test_thread;

/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request();


static void timer_callback(void* data);
static void init();


PROCESS(aa_process, "AA");
AUTOSTART_PROCESSES(&aa_process);

PROCESS_THREAD(aa_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();


  aa_snd_event = process_alloc_event();
  
  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_aa, VIP_AA_URL);

  cooja_mtarch_start(&test_thread, timer_callback, NULL);
  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == aa_snd_event) {
        vip_push_snd_buf((vip_message_t*)data);
        // init();
        cooja_mtarch_exec(&test_thread);
      }
  }
  /* for complie */
  init();
  vip_request();
  PROCESS_END();
}



static void
timer_callback(void* data)
{
  printf("SEND!\n");
  //vip_request();
}

static void init()
{
  int random_incount;
  random_incount = random_rand() % 200 + 100;
  printf("Set Send Timer %d\n", random_incount);

  ctimer_set(&ct, 3, timer_callback, NULL);
}


static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  coap_request_state_t *state = &res_callback_state->state;
  /* Process ack-pkt from vg */
  if (state->status == COAP_REQUEST_STATUS_RESPONSE)
  {
    const char *start = NULL;
    const char *transmit = NULL;
    vip_message_t rcv_ack[1];

    printf("[RES] Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
    if (state->response->code < 100 && state->response->payload_len)
    {
      if (vip_parse_common_header(rcv_ack, state->response->payload, state->response->payload_len) != VIP_NO_ERROR)
      {
        printf("VIP: Not VIP Packet\n");
        return;
      }

      if (coap_get_query_variable(state->response, "start", &start))
      {
        rcv_ack->start_time = atoi(start);
        printf("rcvd start time: %d\n", rcv_ack->start_time);
      }

      if (coap_get_query_variable(state->response, "transmit", &transmit))
      {
        rcv_ack->transmit_time = atoi(transmit);
        printf("rcvd transmit time: %d\n", rcv_ack->transmit_time);
      }

      vip_route(rcv_ack, &aa_type_handler);
    }
  }
}

static void
vip_request() {
  /* set vip endpoint */
  while(!vip_is_empty())
  {
    snd_pkt = vip_front_snd_buf();

    if (snd_pkt->query_len)
    {
      snd_pkt->start_time = RTIMER_NOW()/1000;
      vip_make_query_start_time(snd_pkt->query, snd_pkt->query_len, (uint32_t)snd_pkt->start_time);
      printf("time check! %d | %d\n", snd_pkt->start_time, snd_pkt->transmit_time);
      printf("Query:%s\n", snd_pkt->query);
    }

    coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);

    coap_init_message(request, snd_pkt->re_flag, COAP_POST, 0);
    coap_set_header_uri_path(request, snd_pkt->dest_path);
    coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

    if(snd_pkt->query_len)
      coap_set_header_uri_query(request, snd_pkt->query);

    printf("Send to %s\n", snd_pkt->dest_coap_addr);
    coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);

    vip_pop_snd_buf();
  }
  //cooja_mtarch_yield();
}
