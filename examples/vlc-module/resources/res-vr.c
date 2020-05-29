#include "coap-engine.h"
#include "coap-callback-api.h"
#include "lib/list.h"
#include "vr.h"
#include "cooja_addr.h"
#include "sys/ctimer.h"

/* Node ID */
#include "sys/node-id.h"


#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

static void handler_beacon(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_vrc(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_vsd(vip_message_t *rcv_pkt);

/* Trigger for simul */
static struct ctimer ct;
static int end_flag;
static void trigger_ser(void* data);
static void trigger_vsd(void* data);
static void timer_init(int flag);
static bool is_end_simul();

LIST(session_info);
static void add_new_session(int session_id, int vr_seq);
static void terminate_session(session_t *session);
static void update_session(int session_id, int vr_seq, int vg_seq);
static session_t* check_session(int session_id);

/* for debug */
static void show_session_info();

static vip_message_t snd_pkt[1];
static uint8_t buffer[VIP_MAX_PKT_SIZE];
static char dest_addr[50];
static char query[50];

static int vr_id, aa_id, vt_id;
static int vip_timeout_swtich;
static int loss_count = 0;


/* for simulation */
static int session_id;
static int vr_seq;
static int goal_vg_seq;
int data;

/* vip algorithm */
void retransmit_on();
void retransmit_off();
static void loss_handler();
static bool is_my_vip_pkt(vip_message_t* rcv_pkt);

/* A simple actuator example. Toggles the red led */
EVENT_RESOURCE(res_vr,
         "title=\"vr\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL,
         res_event_handler);


/* vip type handler */
TYPE_HANDLER(vr_type_handler, handler_beacon, NULL, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_vsd, NULL);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Received - mid(%x)\n", request->mid);

  static vip_message_t rcv_pkt[1];
  if (vip_parse_common_header(rcv_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
     printf("vip_pkt have problem\n");
     return;
  }

  vip_route(rcv_pkt, &vr_type_handler);
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  /* check handover and loss */
  if(aa_id != rcv_pkt->aa_id || vt_id != rcv_pkt->vt_id) {

    /* for handover scenario */
    retransmit_off();

    printf("aa(%d) => new aa(%d) | vt(%d) => new vt(%d)\n", aa_id, rcv_pkt->aa_id, vt_id, rcv_pkt->vt_id);
    /* update aa_id, vt_id */
    aa_id = rcv_pkt->aa_id;
    vt_id = rcv_pkt->vt_id;

    /* Recent Received vt-id, aa-id */
    vip_init_message(snd_pkt, VIP_TYPE_VRR, aa_id, vt_id, vr_id);
    vip_set_field_vrr(snd_pkt, 0);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

    vip_init_query(snd_pkt, query);
    vip_make_query_src(query, strlen(query), node_id);
    vip_set_query(snd_pkt, query);

    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
  } else {
    if(vip_timeout_swtich) {
      loss_handler();
    }
  }
}


static void
handler_vra(vip_message_t *rcv_pkt) {
  printf("[vra] rcv_nonce = %d | alloc_vr_id=%d\n", rcv_nonce, vr_id);
  /* received vr-id */
  if(rcv_nonce == rcv_pkt->nonce)
  {
    vr_id = rcv_pkt->vr_id;
    printf("I'm allocated vr-id(%d)!\n", vr_id);
    retransmit_off();

    /* send vrc */
    vip_init_message(snd_pkt, VIP_TYPE_VRC, aa_id, vt_id, vr_id);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);
    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);


    /* trigger for session establish scenario */
    timer_init(0);
  }
}

static void
handler_vrc(vip_message_t *rcv_pkt) {

}


static void
handler_rel(vip_message_t *rcv_pkt) {
  /* developing... */
  terminate_session(0);
  check_session(0);
}

static void
handler_ser(vip_message_t *rcv_pkt) {
  
}

static void
handler_sea(vip_message_t *rcv_pkt) {
  if(!is_my_vip_pkt(rcv_pkt))
    return;

  update_session(rcv_pkt->session_id, 0, rcv_pkt->vg_seq);
  show_session_info();
  goal_vg_seq = rcv_pkt->vg_seq + 100;

  retransmit_off();

  /* send sec = sea's echo */
  vip_init_message(snd_pkt, VIP_TYPE_SEC, aa_id, vt_id, vr_id);
  vip_set_field_sec(snd_pkt, rcv_pkt->session_id, rcv_pkt->vg_seq);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);

  /* trigger of vsd */
  timer_init(1);
}

static void
handler_sec(vip_message_t *rcv_pkt) {

}

static void
handler_vsd(vip_message_t *rcv_pkt) {
  if(!is_my_vip_pkt(rcv_pkt) && is_end_simul(end_flag))
    return;

  retransmit_off();

  session_t *chk;
  if ((chk = check_session(rcv_pkt->session_id)))
  {
    printf("cur vg_seq(%d) <====> rcvd vg_seq(%d)\n", chk->vg_seq, rcv_pkt->seq);
    if (rcv_pkt->seq == chk->vg_seq)
    {
      // Next vg seq data
      chk->vg_seq++;
      // Next to send data to vg
      chk->vr_seq++;


      // next payload
      chk->test_data++;
      char payload[101];
      memset(payload, chk->test_data, 100);

      vip_init_message(snd_pkt, VIP_TYPE_VSD, aa_id, vt_id, vr_id);
      vip_set_field_vsd(snd_pkt, chk->session_id, chk->vr_seq, (void *)payload, 100);
      vip_serialize_message(snd_pkt, buffer);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

      vip_init_query(snd_pkt, query);
      vip_make_query_src(query, strlen(query), vr_id);

      // If received 100's data, Goal in*/
      if (rcv_pkt->seq == goal_vg_seq)
      {
        printf("--------------------------------------------------------------------------------------------------- Goal\n");
        terminate_session(chk);
        vip_make_query_goal(query, strlen(query), 1);
        end_flag = 1;
      }
      vip_set_query(snd_pkt, query);

      process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
    }
    else if(rcv_pkt->seq < chk->vg_seq && rcv_pkt->seq < goal_vg_seq)
    {
      // AA Handover에서는 vr -> vg 가는 메시지가 씹힐 수 있으므로 이 시나리오가 발생할 수 있음
      char payload[101];
      memset(payload, chk->test_data, 100);

      printf("-- Dup Data\n");
      vip_init_message(snd_pkt, VIP_TYPE_VSD, aa_id, vt_id, vr_id);
      vip_set_field_vsd(snd_pkt, chk->session_id, chk->vr_seq, (void *)payload, 100);
      vip_serialize_message(snd_pkt, buffer);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

      vip_init_query(snd_pkt, query);
      vip_make_query_src(query, strlen(query), vr_id);
      vip_set_query(snd_pkt, query);

      process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
    }
    
  }
}

static void 
res_event_handler(void) {
  /* retransmit switch */
  retransmit_on();
}

/* ------ vip retransmit ---------------------*/
void
retransmit_on()
{
  vip_timeout_swtich = 1;
}


void
retransmit_off()
{
  vip_timeout_swtich = 0;
  loss_count = 0;
}

static void
loss_handler() {
  ++loss_count;
  /* if the vr received same beacon frame, retransmit the pkt */
  if(loss_count >= 5) {
    /* Send recently sent pkt */
    loss_count = 0;
    printf("--------------------------------------------------------------------------VIP RETRANSMIT\n");
    snd_pkt->re_flag = COAP_TYPE_NON;
    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
  }
}


/* --------------------- Trigger for simulation -----------------*/

static void trigger_ser(void* data)
{
  session_id = rand();
  vr_seq = rand() % 100000;
  add_new_session(session_id, vr_seq);

  printf("Send SER from vr(%d) with vr_seq(%d)\n", vr_id, vr_seq);
  vip_init_message(snd_pkt, VIP_TYPE_SER, aa_id, vt_id, vr_id);
  vip_set_field_ser(snd_pkt, session_id, vr_seq);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
}

static void trigger_vsd(void* data)
{
    char payload[101];
    memset(payload, 1, 100);

    printf("- START Simulation -\n");
    printf("-- session(%x) - vr_seq(%d) ==> Goal vg_seq(%d) --\n", session_id, vr_seq, goal_vg_seq);
    printf("%s\n", payload);


    vip_init_message(snd_pkt, VIP_TYPE_VSD, aa_id, vt_id, vr_id);
    vip_set_field_vsd(snd_pkt, session_id, vr_seq, (void *)payload, 100);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

    vip_init_query(snd_pkt, query);
    vip_make_query_src(query, strlen(query), vr_id);
    vip_set_query(snd_pkt, query);

    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
}


static void timer_init(int flag)
{
  switch (flag)
  {
  case 0:
    /* session establish scenario */
    ctimer_set(&ct, 30000, trigger_ser, NULL);
    break;
  case 1:
    /* data transmit scenario */
    ctimer_set(&ct, 15000, trigger_vsd, NULL);
    break;
  }
}

static bool is_my_vip_pkt(vip_message_t* rcv_pkt)
{
  if(rcv_pkt->vr_id == vr_id)
    return true;
  return false;
}

static bool is_end_simul(int flag)
{
  if(flag)
    return true;
  else
    return false;
  
}



/* ------------------------- handle session --------------------*/
static void add_new_session(int session_id, int vr_seq)
{
    session_t* new = calloc(1, sizeof(session_t));
    new->session_id = session_id;
    new->vr_seq = vr_seq;
    new->test_data = 1;
    list_add(session_info, new);
}

static void terminate_session(session_t* session)
{
  free(session);
}

static void update_session(int session_id, int vr_seq, int vg_seq)
{
  session_t *cur;
  for(cur=list_head(session_info); cur != NULL; cur = cur->next)
  {
    if(cur->session_id == session_id)
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

static session_t* check_session(int session_id)
{
  session_t *cur;
  for(cur=list_head(session_info); cur != NULL; cur = cur->next)
  {
    if(cur->session_id == session_id)
    {
        return cur;
    }
  }

  return NULL;
}

static void show_session_info()
{
  session_t *cur;
  for(cur=list_head(session_info); cur != NULL; cur = cur->next)
  {
      printf("Current Session Info\n");
      printf("vr(%d) - session(%x) - vr_seq(%d) - vg_seq(%d)\n", cur->vr_id, cur->session_id, cur->vr_seq, cur->vg_seq);
  }

}
