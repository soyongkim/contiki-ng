
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aa.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "net/netstack.h"
#include "sys/ctimer.h"

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
static  vip_message_t snd_pkt[1];

static struct ctimer ct;



/* buffer test */
LIST(snd_buf);

void
vip_push_snd_buf(vip_message_t* vip_pkt)
{
    vip_snd_buf_t* new = malloc(sizeof(vip_snd_buf_t));

    new->total_len = vip_pkt->total_len;
    new->re_flag = vip_pkt->re_flag;

    new->buf = malloc(sizeof(uint8_t)*50);
    new->dest_addr = malloc(sizeof(char)*50);
    new->query = malloc(sizeof(char)*50);
    new->path = malloc(sizeof(char)*50);

    printf("original\n");
    printf("addr:%s\n", vip_pkt->dest_coap_addr);
    printf("query:%s\n", vip_pkt->query);
    printf("path:%s\n", vip_pkt->dest_path);

    memcpy(new->buf, vip_pkt->buffer, 50);
    memcpy(new->dest_addr, vip_pkt->dest_coap_addr, strlen(vip_pkt->dest_coap_addr));
    memcpy(new->query, vip_pkt->query, strlen(vip_pkt->query));
    memcpy(new->path, vip_pkt->dest_path, strlen(vip_pkt->dest_path));

    printf("copy\n");
    printf("addr:%s\n", new->dest_addr);
    printf("query:%s\n", new->query);
    printf("path:%s\n", new->path);


    list_add(snd_buf, new);
}

vip_snd_buf_t*
vip_front_snd_buf()
{
    vip_snd_buf_t* cur = list_head(snd_buf);
    return cur;
}

void
vip_pop_snd_buf()
{
    vip_snd_buf_t* rm = list_head(snd_buf);
    list_remove(snd_buf, rm);
    free(rm->buf);
    free(rm->dest_addr);
    free(rm->query);
    free(rm->path);
    free(rm);
}




/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request(vip_message_t *snd_pkt);


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


  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == aa_snd_event) {
        vip_push_snd_buf((vip_message_t*)data);
        init();
      }
  }

  PROCESS_END();
}



static void
timer_callback(void* data)
{
  printf("SEND!\n");
  vip_snd_buf_t* cur = vip_front_snd_buf();
  vip_pop_snd_buf();

  snd_pkt->total_len = cur->total_len;
  snd_pkt->re_flag = cur->re_flag;

  snd_pkt->buffer = cur->buf;
  snd_pkt->dest_coap_addr = cur->dest_addr;
  snd_pkt->query = cur->query;
  snd_pkt->query_len = strlen(snd_pkt->query);
  snd_pkt->dest_path = cur->path;

  vip_request(snd_pkt);
}

static void init()
{
  int random_incount;
  random_incount = random_rand() % 100;
  printf("Set Send Timer %d\n", random_incount);

  ctimer_set(&ct, random_incount, timer_callback, NULL);
}


static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  coap_request_state_t *state = &res_callback_state->state;
  /* Process ack-pkt from vg */
  if (state->status == COAP_REQUEST_STATUS_RESPONSE)
  {
    vip_message_t rcv_ack[1];

    printf("[RES] Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
    if (state->response->code < 100 && state->response->payload_len)
    {
      if (vip_parse_common_header(rcv_ack, state->response->payload, state->response->payload_len) != VIP_NO_ERROR)
      {
        printf("VIP: Not VIP Packet\n");
        return;
      }
      vip_route(rcv_ack, &aa_type_handler);
    }
  }
}

static void
vip_request(vip_message_t *snd_pkt) {
  /* set vip endpoint */
  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);

  coap_init_message(request, snd_pkt->re_flag, COAP_POST, 0);
  coap_set_header_uri_path(request, snd_pkt->dest_path);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  if(snd_pkt->query_len > 0)
    coap_set_header_uri_query(request, snd_pkt->query);

  printf("Send to %s\n", snd_pkt->dest_coap_addr);
  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
