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
static void handler_sdr(vip_message_t *rcv_pkt);
static void handler_sda(vip_message_t *rcv_pkt);

/* Trigger for simul */
static struct ctimer ct;
static void trigger_ser(void* data);
static void trigger_sdr(void* data);
static void timer_init(int flag);

LIST(session_info);
static void add_new_session(int session_id, int vr_seq);
static void terminate_session(session_t *session);
static void update_session(int session_id, int vr_seq, int vg_seq);
static session_t* check_session(int session_id);


static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char dest_addr[50];
static char query[50];

static int vr_id, aa_id, vt_id;
static int vip_timeout_swtich;
static int loss_count = 0;

void retransmit_on();
void retransmit_off();
static void loss_handler();

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
              handler_sdr, handler_sda, NULL);


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
  update_session(rcv_pkt->session_id, 0, rcv_pkt->vg_seq);

  printf("Thank you! I received vg_seq(%d)\n", rcv_pkt->vg_seq);

  retransmit_off();

  /* send sec = sea's echo */
  vip_init_message(snd_pkt, VIP_TYPE_SEC, aa_id, vt_id, vr_id);
  vip_set_field_sec(snd_pkt, rcv_pkt->session_id, rcv_pkt->vg_seq);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
}

static void
handler_sec(vip_message_t *rcv_pkt) {

}

static void
handler_sdr(vip_message_t *rcv_pkt) {

}

static void
handler_sda(vip_message_t *rcv_pkt) {

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
    printf("----------------------------------VIP RETRANSMIT\n");
    snd_pkt->re_flag = COAP_TYPE_NON;
    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
  }
}



/* ------------------------- handle session --------------------*/
static void add_new_session(int session_id, int vr_seq)
{
    session_t* new = calloc(1, sizeof(session_t));
    new->session_id = session_id;
    new->vr_seq = vr_seq;
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


/* --------------------- Trigger for simulation -----------------*/
static void trigger_ser(void* data)
{
  int session_id = rand();
  int vr_seq = rand() % 100000;
  add_new_session(session_id, vr_seq);

  printf("Session_id: %x\n", session_id);
  printf("vr_seq : %d\n", vr_seq);


  vip_init_message(snd_pkt, VIP_TYPE_SER, aa_id, vt_id, vr_id);
  vip_set_field_ser(snd_pkt, session_id, vr_seq);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
}

static void trigger_sdr(void* data)
{

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
    ctimer_set(&ct, 30000, trigger_sdr, NULL);
    break;
  }
}