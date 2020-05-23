#include "contiki.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "lib/list.h"
#include "aa.h"
#include "cooja_addr.h"
#include "os/sys/mutex.h"

/* Node ID */
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_ad_handler(void);

static void handler_beacon(vip_message_t *rcv_pkt);
static void handler_vrr(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_vrc(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_sd(vip_message_t *rcv_pkt);
static void handler_sda(vip_message_t *rcv_pkt);
static void allocate_vt_handler(vip_message_t *rcv_pkt);

/* make vt table which administrate the vt id */
LIST(vt_table);
LIST(vr_nonce_table);

/* for snd-pkt */
static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char dest_addr[50];
static char query[50];

/* use ack for query */
static vip_message_t ack_pkt[1];
static char ack_query[50];

static int nonce_pool[65000];
static mutex_t p;

static char uplink_id[50] = {"ISL_AA_UPLINK_ID"};

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];



/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request(vip_message_t *snd_pkt);


/* A simple actuator example. Toggles the red led */
PERIODIC_RESOURCE(res_aa,
         "title=\"AA\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL,
         30000,
         res_periodic_ad_handler);


/* vip type handler */
TYPE_HANDLER(aa_type_handler, handler_beacon, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, allocate_vt_handler);

int
publish_nonce() {
  int pick_nonce = 0;
  mutex_try_lock(&p);
  /* publish nonce_pool */
  for(int i=1; i<65000; i++) {
    if(!nonce_pool[i]) {
      nonce_pool[i] = 1;
      pick_nonce = i;
      break;
    }
  }
  mutex_unlock(&p);
  return pick_nonce;
}

void
expire_nonce(int target) {
  nonce_pool[target] = 0;
}

int
add_nonce_table(int vr_node_id) {
  vip_nonce_tuple_t* new_tuple = malloc(sizeof(vip_nonce_tuple_t));
  new_tuple->nonce = publish_nonce();
  new_tuple->vr_node_id = vr_node_id;
  list_add(vr_nonce_table, new_tuple);

  return new_tuple->nonce;
}

void
remove_nonce_table(vip_nonce_tuple_t* tuple) {
  expire_nonce(tuple->nonce);
  list_remove(vr_nonce_table, tuple);
  free(tuple);
}

vip_nonce_tuple_t*
check_nonce_table(int vr_node_id) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_nonce_table); c != NULL; c = c->next) {
    if(c->vr_node_id == vr_node_id) {
        return c;
    }
  }
  return NULL;
}

void
add_vt_tuple(int node_id) {
  vip_vt_tuple_t *new_tuple = malloc(sizeof(vip_vt_tuple_t));
  new_tuple->vt_id = node_id;
  list_add(vt_table, new_tuple);
}

void
remove_vt_tuple(vip_vt_tuple_t* tuple) {
  list_remove(vt_table, tuple);
  free(tuple);
}

int
check_vt_tuple(int vt_id) {
  vip_vt_tuple_t *c;
  for(c = list_head(vt_table); c != NULL; c = c->next) {
    if(c->vt_id == vt_id) {
      return c->vt_id;
    }
  }
  return 0;
}


void show_vt_table() {
  vip_vt_tuple_t *c;
  for(c = list_head(vt_table); c != NULL; c = c->next) {
    printf("[aa(%d) -> vt(%d):]\n", node_id, c->vt_id);
  }
}


void
handover_vr(vip_message_t* rcv_pkt) {
  /* forward to vg */
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
  printf("forward to vg(%d)\n", VIP_VG_ID);
  vip_request(rcv_pkt);
}

/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *src = NULL;
  printf("Received - mid(%x)\n", request->mid);

  const uint8_t *chunk;
  int len = coap_get_payload(request, &chunk);
  buffer = chunk;
  
  static vip_message_t rcv_pkt[1];
  if (vip_parse_common_header(rcv_pkt, buffer, len) != VIP_NO_ERROR)
  {
     printf("vip_pkt have problem\n");
     return;
  }

  if(coap_get_query_variable(request, "src", &src)) {
    rcv_pkt->query_rcv_id = atoi(src);
  }

  vip_route(rcv_pkt, &aa_type_handler);

  /* for ack */
  if(ack_pkt->total_len)
    coap_set_payload(response, ack_pkt->buffer, ack_pkt->total_len);
  if(ack_pkt->query_len)
    coap_set_header_uri_query(response, ack_pkt->query);
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  printf("I'm beacon handler [%s]\n", rcv_pkt->uplink_id);
}


static void
handler_vrr(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  int nonce;
  if (!(chk = check_nonce_table(rcv_pkt->query_rcv_id)))
  {
    /* publish new nonce to the vr */
    nonce = add_nonce_table(rcv_pkt->query_rcv_id);

    /* Send vrr to vg */
    printf("forward to vg..\n");
    vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_set_type_header_nonce(rcv_pkt, nonce);
    vip_serialize_message(rcv_pkt, buffer);
    vip_request(rcv_pkt);
  }
  else
  {
    nonce = chk->nonce;
  }

  /* Set payload for ack */
  printf("Setting Ack..\n");
  vip_init_query(query);
  vip_make_query_nonce(ack_query, strlen(ack_query), nonce);
  vip_set_query(ack_pkt, ack_query);
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  /* forward vra(vrid) to vt with nonce*/
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
  vip_serialize_message(rcv_pkt, buffer);
  vip_request(rcv_pkt);
}

static void
handler_vrc(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  /* if vrc is duplicated, the tuple is null. so nothing to do and just send ack */
  if (!(chk = check_nonce_table(rcv_pkt->query_rcv_id)))
  {
    /* remove nonce tuple if vrc received */
    remove_nonce_table(chk);

    /* forward vra(vrid) to vt with nonce*/
    vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_request(rcv_pkt);
  }
}

static void
handler_rel(vip_message_t *rcv_pkt) {

}

static void
handler_ser(vip_message_t *rcv_pkt) {

}

static void
handler_sea(vip_message_t *rcv_pkt) {

}

static void
handler_sec(vip_message_t *rcv_pkt) {

}

static void
handler_sd(vip_message_t *rcv_pkt) {

}

static void
handler_sda(vip_message_t *rcv_pkt) {

}

static void
allocate_vt_handler(vip_message_t *rcv_pkt) {
  printf("alloc handler\n");
  if(!check_vt_tuple(rcv_pkt->vt_id)) {
    add_vt_tuple(rcv_pkt->vt_id);
    show_vt_table();
  }
}


static void
res_periodic_ad_handler(void)
{
  printf("Advertise...\n");

  /* pkt, type, aa-id(node_id), vt-id */
  vip_init_message(snd_pkt, VIP_TYPE_ALLOW, node_id, 0, 0);
  vip_set_payload(snd_pkt, (void *)uplink_id, strlen(uplink_id));
  vip_serialize_message(snd_pkt, buffer);

  vip_set_dest_ep_cooja(snd_pkt, dest_addr, VIP_BROADCAST, VIP_VT_URL);

  /* set query */
  vip_init_query(query);
  vip_make_query_src(query, strlen(query), node_id);
  vip_set_query(snd_pkt, query);

  vip_request(snd_pkt);
}


static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  coap_request_state_t *state = &res_callback_state->state;
  /* Process ack-pkt from vg */
  if (state->status == COAP_REQUEST_STATUS_RESPONSE)
  {
    const uint8_t *chunk;

    vip_message_t rcv_ack[1];

    int len = coap_get_payload(state->response, &chunk);
    printf("Ack:%d - mid(%x) - payload_len(%d) - req?(%d)\n", state->response->code, state->response->mid, len, state->request->payload_len);
    if (state->response->code < 100 && len)
    {
      if (vip_parse_common_header(rcv_ack, chunk, len) != VIP_NO_ERROR)
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
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(request, snd_pkt->dest_path);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  if(snd_pkt->query_len > 0)
    coap_set_header_uri_query(request, snd_pkt->query);

  printf("Send from %s to %s\n", snd_pkt->query, snd_pkt->dest_coap_addr);
  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
