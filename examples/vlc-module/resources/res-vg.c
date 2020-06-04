#include "coap-engine.h"
#include "coap-callback-api.h"
#include "lib/list.h"
#include "vg.h"
#include "cooja_addr.h"
#include "os/sys/mutex.h"

/* Node ID */
#include "sys/node-id.h"
#include "sys/rtimer.h"

#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

static void handler_vrr(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_vrc(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_vsd(vip_message_t *rcv_pkt);


LIST(vr_table);
int publish_vrid();
void expire_vrid(int target);
int add_vr_table(int aa_id, int nonce);
void remove_vr_table(vip_nonce_tuple_t* tuple);
vip_nonce_tuple_t* check_vr_table(int aa_id, int nonce);


LIST(session_info);
static void add_new_session(int vr_id, int session_id, int vr_seq, int vg_seq);
static void terminate_session(session_t *session);
static void update_session(int vr_id, int session_id, int vr_seq, int vg_seq);
static session_t* check_session(int vr_id);

/* for debug */
static void show_session_info();



/* for snd-pkt */
static uint8_t buffer[VIP_MAX_PKT_SIZE];

/* use ack for query */
static vip_message_t ack_pkt[1];
static char ack_query[50];

static mutex_t v;

static int goal_flag;


/* vr session_array */
/* array index is "VR-ID" */
//static vip_vr_session_tuple_t session_arr[65000];
/* vr id pool */
static int vr_id_pool[65000];

/* A simple actuator example. Toggles the red led */
EVENT_RESOURCE(res_vg,
         "title=\"VG\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL,
         res_event_handler);

/* vip type handler */
TYPE_HANDLER(vg_type_handler, NULL, handler_vrr, handler_vra, 
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
      rcv_pkt->start_time = atoi(start);
      printf("rcvd start time: %u\n", rcv_pkt->start_time);
  }

  if(coap_get_query_variable(request, "transmit", &transmit))
  {
      rcv_pkt->transmit_time = atoi(transmit);
      printf("rcvd transmit time: %u\n", rcv_pkt->transmit_time);
  }

  vip_route(rcv_pkt, &vg_type_handler);

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
res_event_handler(void) {
  printf("Init VG..\n");
}

static void
handler_vrr(vip_message_t *rcv_pkt)
{
  /* case handover */
  if(rcv_pkt->vr_id)
  {
    session_t* chk;
    if((chk = check_session(rcv_pkt->vr_id)))
    {
      printf("handover vr(%d)! => send to aa(%d) - vt(%d)\n", rcv_pkt->vr_id, rcv_pkt->aa_id, rcv_pkt->vt_id);
      /* last received vsd */
      char payload[101];
      memset(payload, chk->test_data-1, 100);

      vip_init_message(ack_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
      vip_set_field_vsd(ack_pkt, chk->session_id, chk->vg_seq-1, (void *)payload, 100);
      vip_serialize_message(ack_pkt, buffer);
    }
    return;
  }


  /* case allocation */
  vip_nonce_tuple_t *chk;
  int alloc_vr_id;
  if (!(chk = check_vr_table(rcv_pkt->aa_id, rcv_pkt->nonce)))
  {
    /* publish new vr-id */
    alloc_vr_id = add_vr_table(rcv_pkt->aa_id, rcv_pkt->nonce);
  }
  else
  {
    alloc_vr_id = chk->alloc_vr_id;
  }

  /* Set payload for ack */
  printf("Setting Ack..\n");
  vip_init_message(ack_pkt, VIP_TYPE_VRA, rcv_pkt->aa_id, rcv_pkt->vt_id, alloc_vr_id);
  vip_set_field_vra(ack_pkt, rcv_pkt->nonce);
  vip_serialize_message(ack_pkt, buffer);
}

static void
handler_vra(vip_message_t *rcv_pkt) {


}

static void
handler_vrc(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  /* if vrc is duplicated, the tuple is null. so nothing to do and just send ack */
  if ((chk = check_vr_table(rcv_pkt->aa_id, rcv_pkt->vr_id)))
  {
    /* complete allocation. for the other aa, delete nonce value*/
    chk->nonce = 0;
    printf("vr[%d] complete!\n", rcv_pkt->vr_id);
  }
}

static void
handler_rel(vip_message_t *rcv_pkt) {
  /* not used function collect */
  terminate_session(check_session(0));
  update_session(0, 0, 0, 0);
}

static void
handler_ser(vip_message_t *rcv_pkt) {
  session_t *chk;
  int vg_seq;
  if (!(chk = check_session(rcv_pkt->vr_id)))
  {
    /* publish new vr-id */
    vg_seq = rand() % 100000;
    add_new_session(rcv_pkt->vr_id, rcv_pkt->session_id, rcv_pkt->vr_seq, vg_seq);
    printf("[vr(%d) - session(%x) - vr_seq(%d)] ==> vg_seq(%d)\n", rcv_pkt->vr_id, rcv_pkt->session_id, rcv_pkt->vr_seq, vg_seq);
  }
  else
  {
    vg_seq = chk->vg_seq;
  }

  /* Set payload for ack */
  printf("Setting Ack..\n");
  vip_init_message(ack_pkt, VIP_TYPE_SEA, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
  vip_set_field_sea(ack_pkt, rcv_pkt->session_id, vg_seq);
  vip_serialize_message(ack_pkt, buffer);
}

static void
handler_sea(vip_message_t *rcv_pkt) {

}

static void
handler_sec(vip_message_t *rcv_pkt) {
  printf("Establish Session(%x) - vr(%d)\n", rcv_pkt->session_id, rcv_pkt->vr_id);
  show_session_info();
}

static void
handler_vsd(vip_message_t *rcv_pkt) {
    if(rcv_pkt->start_time)
    {
      uint32_t cur_time = RTIMER_NOW()/1000;
      uint32_t time_hop = cur_time - rcv_pkt->start_time;
      printf("cur time: %u\n", cur_time);
      printf("rcv start time: %u\n", rcv_pkt->start_time);
      printf("time hop to hop: %u\n", time_hop);
      rcv_pkt->transmit_time += time_hop;
      printf("time to vg: %u\n", rcv_pkt->transmit_time);
      rcv_pkt->start_time = cur_time;
    }

    session_t *chk;
    if((chk = check_session(rcv_pkt->vr_id)))
    {
      if(goal_flag)
      {
        terminate_session(chk);
        return;
      }

      if(rcv_pkt->seq == chk->vr_seq)
      {
        printf("Current state - vr(%d) <= vr_seq(%d) / vg_seq(%d)\n", rcv_pkt->vr_id, chk->vr_seq, chk->vg_seq);
        chk->vr_seq++;

        chk->test_data++;
        char payload[101];
        memset(payload, chk->test_data, 100);

        vip_init_message(ack_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
        vip_set_field_vsd(ack_pkt, chk->session_id, chk->vg_seq++, (void *)payload, 100);
        vip_serialize_message(ack_pkt, buffer);
      }
      else
      {
        char payload[101];
        memset(payload, chk->test_data-1, 100);

        vip_init_message(ack_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
        vip_set_field_vsd(ack_pkt, chk->session_id, chk->vg_seq-1, (void *)payload, 100);
        vip_serialize_message(ack_pkt, buffer);
      }

      vip_init_query(ack_pkt, ack_query);
      vip_make_query_start_time(ack_query, strlen(ack_query), rcv_pkt->start_time);
      vip_make_query_transmit_time(ack_query, strlen(ack_query), rcv_pkt->transmit_time);
      vip_set_query(ack_pkt, ack_query);
    }
}


/* --------------handle vrid-----------------------------------------*/
int
publish_vrid() {
  mutex_try_lock(&v);
  int vr_id = 0;
  for(int i=1; i<65000; i++) {
    if(!vr_id_pool[i]) {
      vr_id_pool[i] = 1;
      vr_id = i;
      break;
    }
  }
  mutex_unlock(&v);
  return vr_id;
}

void
expire_vrid(int target) {
  vr_id_pool[target] = 0;
}

int
add_vr_table(int aa_id, int nonce) {
  vip_nonce_tuple_t* new_tuple = malloc(sizeof(vip_nonce_tuple_t));
  new_tuple->alloc_vr_id = publish_vrid();
  new_tuple->aa_id = aa_id;
  new_tuple->nonce = nonce;
  list_add(vr_table, new_tuple);

  return new_tuple->alloc_vr_id;
}

void
remove_vr_table(vip_nonce_tuple_t* tuple) {
  list_remove(vr_table, tuple);
  free(tuple);
}

vip_nonce_tuple_t*
check_vr_table(int aa_id, int nonce) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_table); c != NULL; c = c->next) {
    if(c->aa_id == aa_id && c->nonce == nonce) {
        return c;
    }
  }
  return NULL;
}


/* ------------------------- handle session --------------------*/
static void add_new_session(int vr_id, int session_id, int vr_seq, int vg_seq)
{
    session_t* new = calloc(1, sizeof(session_t));
    new->vr_id = vr_id;
    new->session_id = session_id;
    new->vr_seq = vr_seq;
    new->vg_seq = vg_seq;
    new->test_data = 0;
    list_add(session_info, new);
}

static void terminate_session(session_t* session)
{
  list_remove(session_info, session);
  session->vr_id = 0;
  free(session);
}

static void update_session(int vr_id, int session_id, int vr_seq, int vg_seq)
{
  session_t *cur;
  for(cur=list_head(session_info); cur != NULL; cur = cur->next)
  {
    if(cur->vr_id == vr_id && cur->session_id == session_id)
    {
      if(vr_seq)
      {
        cur->vr_seq = vr_seq;
      }

      if(vg_seq)
      {
        cur->vg_seq = vg_seq;
      }
    }
  }
}

static session_t* check_session(int vr_id)
{
  session_t *cur;
  for(cur=list_head(session_info); cur != NULL; cur = cur->next)
  {
    if(cur->vr_id == vr_id)
    {
        return cur;
    }
  }

  return NULL;
}

static void show_session_info()
{
  printf("Current Session Info\n");
  session_t *cur;
  for(cur=list_head(session_info); cur != NULL; cur = cur->next)
  {
      printf("vr(%d) - session(%x) - vr_seq(%d) - vg_seq(%d)\n", cur->vr_id, cur->session_id, cur->vr_seq, cur->vg_seq);
  }

}