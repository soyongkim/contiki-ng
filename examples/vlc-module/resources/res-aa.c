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

/* test pthread */
#include "os/sys/pt.h"


#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_ad_handler(void);

static void handler_vrr(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_vrc(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_sd(vip_message_t *rcv_pkt);
static void handler_sda(vip_message_t *rcv_pkt);

/* make vt table which administrate the vt id */
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
//static mutex_t p;

static char uplink_id[50] = {"ISL_AA_UPLINK_ID"};


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
TYPE_HANDLER(aa_type_handler, NULL, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, NULL);

int
publish_nonce() {
  int pick_nonce = 0;
  /* publish nonce_pool */
  for(int i=1; i<65000; i++) {
    if(!nonce_pool[i]) {
      nonce_pool[i] = 1;
      pick_nonce = i;
      break;
    }
  }
  return pick_nonce;
}

void
expire_nonce(int target) {
  nonce_pool[target] = 0;
}

int
add_nonce_table(int vr_node_id) {
  //mutex_try_lock(&p);
  vip_nonce_tuple_t* new_tuple = malloc(sizeof(vip_nonce_tuple_t));
  new_tuple->nonce = publish_nonce();
  new_tuple->vr_node_id = vr_node_id;
  new_tuple->alloc_vr_id = 0;
  list_add(vr_nonce_table, new_tuple);
  //mutex_unlock(&p);
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
update_nonce_table(int nonce, int vr_id) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_nonce_table); c != NULL; c = c->next) {
    if(c->nonce == nonce) {
        c->alloc_vr_id = vr_id;
        printf("Update nonce(%d) - vr(%d)\n", c->nonce, c->alloc_vr_id);
        break;
    }
  }
}

void
handover_vr(vip_message_t* rcv_pkt) {
  /* forward to vg */
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
  printf("forward to vg(%d)\n", VIP_VG_ID);
  process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
}

/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("A: [Test] Name:%s | Thread:%s\n",PROCESS_CURRENT()->name, PROCESS_CURRENT()->thread);

  const char *src = NULL;
  printf("Received - mid(%x)\n", request->mid);

  static vip_message_t rcv_pkt[1];
  if (vip_parse_common_header(rcv_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("VIP: Not VIP Packet\n");
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
  {
    coap_set_header_uri_query(response, ack_pkt->query);
  }
  printf("B: [Test] Name:%s | Thread:%s\n",PROCESS_CURRENT()->name, PROCESS_CURRENT()->thread);
}

static void
handler_vrr(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  int nonce;
  if (!(chk = check_nonce_table(rcv_pkt->query_rcv_id)))
  {
    /* publish new nonce to the vr */
    nonce = add_nonce_table(rcv_pkt->query_rcv_id);
    printf("Pub(%d) to vr(%d)\n", nonce, rcv_pkt->query_rcv_id);

    /* Send vrr to vg */
    printf("forward to vg..\n");
    vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_set_type_header_nonce(rcv_pkt, nonce);
    vip_serialize_message(rcv_pkt, buffer);
    process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
  }
  else
  {
    nonce = chk->nonce;

    printf("forward to vt..\n");
    if(chk->alloc_vr_id)
    {
      printf("vt? %d\n", rcv_pkt->vt_id);
      vip_init_message(snd_pkt, VIP_TYPE_VRA, node_id, rcv_pkt->vt_id, chk->alloc_vr_id);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
      vip_set_type_header_nonce(snd_pkt, nonce);
      vip_serialize_message(snd_pkt, buffer);
      process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
    }
  }

  /* Set payload for ack */
  vip_init_query(ack_query);
  vip_make_query_nonce(ack_query, strlen(ack_query), nonce);
  vip_set_query(ack_pkt, ack_query);
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  update_nonce_table(rcv_pkt->nonce, rcv_pkt->vr_id);
  /* forward vra(vrid) to vt with nonce*/
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
  vip_serialize_message(rcv_pkt, buffer);
  process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
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
    process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
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


  /* non message setting */
  snd_pkt->re_flag = COAP_TYPE_NON;

  process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
}