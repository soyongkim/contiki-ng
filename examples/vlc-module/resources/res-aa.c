#include "coap-engine.h"
#include "coap-callback-api.h"
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

/* coap handler */
static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_ad_handler(void);

/* vip handler */
static void handler_vrr(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_vrc(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_vsd(vip_message_t *rcv_pkt);

/* vr nonce function */
/* nonce pub */
int publish_nonce();
void expire_nonce(int nonce);

/* handle vr_cache */
int add_vr_cache(int vr_node_id);
void remove_vr_cache(vip_nonce_tuple_t* tuple);
void update_vr_cache(int nonce, int vr_id);
vip_nonce_tuple_t* check_vr_cache_node_id(int vr_node_id);
vip_nonce_tuple_t *check_vr_cache_vr_id(int vr_id);


/* handle se_cache */
void add_se_cache(int vr_id, int session_id);
void remove_se_cache(se_cache_t* tuple);
void update_se_cache(int vr_id, int session_id, int vg_seq);
se_cache_t* check_se_cache(int vr_id, int session_id);




/* VR Register Cache */
LIST(vr_cache);

/* Session Establish Cache */
LIST(se_cache);

/* for snd-pkt */
static vip_message_t snd_pkt[1];
static uint8_t buffer[VIP_MAX_PKT_SIZE];
static char dest_addr[50];
static char query[50];

/* use ack for query */
static vip_message_t ack_pkt[1];
static char ack_query[50];

static int nonce_pool[65000];

static int goal_flag;

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
              handler_vsd, NULL);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *src = NULL;
  const char *goal = NULL;
  const char *start = NULL;
  const char *transmit = NULL;
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

  if(coap_get_query_variable(request, "goal", &goal))
  {
      printf("--------------------------------------------------------------------------------------------------- Goal\n");
      goal_flag = 1;
  }

  if(coap_get_query_variable(request, "start", &start))
  {
      rcv_pkt->start_time = atol(start);
      printf("rcvd start time: %ld\n", rcv_pkt->start_time);
  }

  if(coap_get_query_variable(request, "transmit", &transmit))
  {
      rcv_pkt->transmit_time = atol(transmit);
      printf("rcvd transmit time: %ld\n", rcv_pkt->transmit_time);
  }



  vip_route(rcv_pkt, &aa_type_handler);

  /* for ack */
  if(ack_pkt->total_len)
  {
    coap_set_payload(response, ack_pkt->buffer, ack_pkt->total_len);
    ack_pkt->total_len = 0;
  }
  if(ack_pkt->query_len)
  {
    coap_set_header_uri_query(response, ack_pkt->query);
    ack_pkt->query_len = 0;
  }
}

static void
handler_vrr(vip_message_t *rcv_pkt) {
  /* handover sceanario */
  if(rcv_pkt->vr_id)
  {
    printf("handover vr(%d)\n", rcv_pkt->vr_id);
    vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_serialize_message(rcv_pkt, buffer);
    process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
    return;
  }


  /* allocation sceanario */
  vip_nonce_tuple_t *chk;
  int nonce;
  if (!(chk = check_vr_cache_node_id(rcv_pkt->query_rcv_id)))
  {
    /* publish new nonce to the vr */
    nonce = add_vr_cache(rcv_pkt->query_rcv_id);
    printf("Pub(%d) to vr(%d)\n", nonce, rcv_pkt->query_rcv_id);

    /* Send vrr to vg */
    vip_init_message(snd_pkt, VIP_TYPE_VRR, node_id, rcv_pkt->vt_id, 0);
    vip_set_field_vrr(snd_pkt, nonce);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
  }
  else
  {
    nonce = chk->nonce;

    if(chk->alloc_vr_id)
    {
      vip_init_message(snd_pkt, VIP_TYPE_VRA, node_id, rcv_pkt->vt_id, chk->alloc_vr_id);
      vip_set_field_vra(snd_pkt, nonce);
      vip_serialize_message(snd_pkt, buffer);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
      process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
    }
  }

  /* Set payload for ack */
  vip_init_query(snd_pkt, ack_query);
  vip_make_query_nonce(ack_query, strlen(ack_query), nonce);
  vip_make_query_timer(ack_query, strlen(ack_query), 1);
  vip_set_query(ack_pkt, ack_query);
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  update_vr_cache(rcv_pkt->nonce, rcv_pkt->vr_id);
  /* forward vra(vrid) to vt with nonce*/
  vip_init_message(snd_pkt, VIP_TYPE_VRA, node_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
  vip_set_field_vra(snd_pkt, rcv_pkt->nonce);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
  process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
}

static void
handler_vrc(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  /* if vrc is duplicated, the tuple is already freed. so nothing to do and just send ack */
  if ((chk = check_vr_cache_vr_id(rcv_pkt->vr_id)))
  {
    printf("received rcv! from vr(%d)\n", chk->alloc_vr_id);
    /* remove nonce tuple if vrc received */
    remove_vr_cache(chk);

    /* forward vra(vrid) to vt with nonce*/
    vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_serialize_message(rcv_pkt, buffer);
    process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
  }
}

static void
handler_rel(vip_message_t *rcv_pkt) {

}

static void
handler_ser(vip_message_t *rcv_pkt) {
  se_cache_t *chk;
  if (!(chk = check_se_cache(rcv_pkt->vr_id, rcv_pkt->session_id)))
  {
    /* cache miss.. */
    add_se_cache(rcv_pkt->vr_id, rcv_pkt->session_id);
    printf("Add se_cache vr(%d) - session(%x)\n", rcv_pkt->vr_id, rcv_pkt->session_id);

    /* Send ser to vg */
    vip_init_message(snd_pkt, VIP_TYPE_SER, node_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
    vip_set_field_ser(snd_pkt, rcv_pkt->session_id, rcv_pkt->vr_seq);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
  }
  else
  {
    /* cache hit !*/
    if(!chk->vg_seq)
    {
        printf("Case that the vr retransmit before forward previous ser pkt\n");
    }
    else
    {
      /* Case that the sea msg is already arrived to VG */
      /* cache data send to vt */
      vip_init_message(snd_pkt, VIP_TYPE_SEA, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
      vip_set_field_sea(snd_pkt, chk->session_id, chk->vg_seq);
      vip_serialize_message(snd_pkt, buffer);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
      process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
    }
  }

  vip_init_query(ack_pkt, ack_query);
  vip_make_query_timer(ack_query, strlen(ack_query), 1);
  vip_set_query(ack_pkt, ack_query);
}

static void
handler_sea(vip_message_t *rcv_pkt) {
  update_se_cache(rcv_pkt->vr_id, rcv_pkt->session_id, rcv_pkt->vg_seq);
  /* forward sea to vt with vg_seq*/
  vip_init_message(snd_pkt, VIP_TYPE_SEA, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
  vip_set_field_sea(snd_pkt, rcv_pkt->session_id, rcv_pkt->vg_seq);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
  process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
}

static void
handler_sec(vip_message_t *rcv_pkt) {
  se_cache_t *chk;
  /* if vrc is duplicated, the tuple is already freed. so nothing to do and just send ack */
  if ((chk = check_se_cache(rcv_pkt->vr_id, rcv_pkt->session_id)))
  {
    printf("received rcv! from vr(%d)\n", chk->vr_id);
    /* remove nonce tuple if vrc received */
    remove_se_cache(chk);

    /* forward vra(vrid) to vt with nonce*/
    vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_serialize_message(rcv_pkt, buffer);
    process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
  }
}

static void
handler_vsd(vip_message_t *rcv_pkt) {
    if(rcv_pkt->start_time)
    {
      rcv_pkt->transmit_time += clock_seconds() - rcv_pkt->start_time;
      printf("time to aa: %ld\n", rcv_pkt->transmit_time);
    }

    if(rcv_pkt->query_rcv_id)
    {
      if (goal_flag)
      {
        /* forward goal flag to vg */
        vip_init_query(rcv_pkt, query);
        vip_make_query_goal(query, strlen(query), 1);
        vip_set_query(rcv_pkt, query);
        printf("Q: %s\n", query);
      }
      else
      {
        /* if not goal, turn on vr timer */
        vip_init_query(ack_pkt, ack_query);
        vip_make_query_timer(ack_query, strlen(ack_query), 1);
        vip_set_query(ack_pkt, ack_query);
      }

      // arrived from vr
      vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_VG_ID, VIP_VG_URL);
      vip_serialize_message(rcv_pkt, buffer);
      process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
    }
    else
    {
      // arrived from vg
      vip_set_dest_ep_cooja(rcv_pkt, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
      vip_serialize_message(rcv_pkt, buffer);
      process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);    
    }
}

static void
res_periodic_ad_handler(void)
{
  printf("Advertise...\n");

  /* pkt, type, aa-id(node_id), vt-id */
  vip_init_message(snd_pkt, VIP_TYPE_ALLOC, node_id, 0, 0);
  vip_set_field_alloc(snd_pkt, uplink_id);
  vip_serialize_message(snd_pkt, buffer);

  vip_set_dest_ep_cooja(snd_pkt, dest_addr, VIP_BROADCAST, VIP_VT_URL);

  /* non message setting */
  vip_set_non_flag(snd_pkt);

  process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
}

/* ----------------------------- vr cache function ------------------------------------------*/
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
add_vr_cache(int vr_node_id) {
  //mutex_try_lock(&p);
  vip_nonce_tuple_t* new_tuple = malloc(sizeof(vip_nonce_tuple_t));
  new_tuple->nonce = publish_nonce();
  new_tuple->vr_node_id = vr_node_id;
  new_tuple->alloc_vr_id = 0;
  list_add(vr_cache, new_tuple);
  //mutex_unlock(&p);
  return new_tuple->nonce;
}

void
remove_vr_cache(vip_nonce_tuple_t* tuple) {
  expire_nonce(tuple->nonce);
  list_remove(vr_cache, tuple);
  free(tuple);
}

vip_nonce_tuple_t*
check_vr_cache_node_id(int vr_node_id) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_cache); c != NULL; c = c->next) {
    if(c->vr_node_id == vr_node_id) {
        return c;
    }
  }
  return NULL;
}

vip_nonce_tuple_t *
check_vr_cache_vr_id(int vr_id)
{
  vip_nonce_tuple_t *c;
  for (c = list_head(vr_cache); c != NULL; c = c->next)
  {
    if (c->alloc_vr_id == vr_id)
    {
      return c;
    }
  }
  return NULL;
}

void
update_vr_cache(int nonce, int vr_id) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_cache); c != NULL; c = c->next) {
    if(c->nonce == nonce) {
        c->alloc_vr_id = vr_id;
        printf("Update vr_cache : nonce(%d) - vr(%d)\n", c->nonce, c->alloc_vr_id);
        break;
    }
  }
}


/*----------------------------- se cache function ------------------------------------------*/

void add_se_cache(int vr_id, int session_id)
{
  se_cache_t* new = calloc(1, sizeof(se_cache_t));
  new->vr_id = vr_id;
  new->session_id = session_id;
  list_add(se_cache, new);
}

void remove_se_cache(se_cache_t* tuple)
{
  list_remove(se_cache, tuple);
}

void update_se_cache(int vr_id, int session_id, int vg_seq)
{
  se_cache_t* cur;
  for(cur = list_head(se_cache); cur != NULL; cur = cur->next)
  {
    if(cur->vr_id == vr_id && cur->session_id == session_id)
    {
      cur->vg_seq = vg_seq;
      printf("Update se_cache : vr(%d)'s session(%x) - vg_seq(%d)\n", vr_id, session_id, vg_seq);
      break;
    }
  }
}

se_cache_t* check_se_cache(int vr_id, int session_id)
{
  se_cache_t* cur;
  for(cur = list_head(se_cache); cur != NULL; cur = cur->next)
  {
    if(cur->vr_id == vr_id && cur->session_id == session_id)
    {
      return cur;
    }
  }

  return NULL;
}

