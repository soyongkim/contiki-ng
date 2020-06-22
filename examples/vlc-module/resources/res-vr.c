#include "coap-engine.h"
#include "coap-callback-api.h"
#include "lib/list.h"
#include "vr.h"
#include "cooja_addr.h"
#include "sys/ctimer.h"
#include "time.h"

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
static void trigger_ser(void* data);
static void trigger_vsd(void* data);
static void timer_init(int flag);

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
static char query[VIP_MAX_QUERY_SIZE];

static int vr_id, aa_id, vt_id;
static int vip_timeout_swtich;
static int simul_end;


/* for simulation */
static int session_id;
static int vr_seq;
int data;
static unsigned long ho_init_time;

static int cumul_ack;
static int init_seq;
static int last_seq;

static int ack_flag;
static int out_of_order_flag;

static int dup_cnt;
static int consecutive_cnt;
static int simul_buffer[VIP_SIMUL_DATA];
static int gap_num;
static uint32_t gap_list[VIP_SIMUL_DATA];


/* vip algorithm */
void retransmit_on();
void retransmit_off();
static bool is_my_vip_pkt(vip_message_t* rcv_pkt);

void sliding_window_handler(vip_message_t* rcv_pkt);
void sliding_window_loss_search();
void sliding_window_send_ack();

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
              handler_vsd, NULL, NULL);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *start = NULL;
  const char *transmit = NULL;
  printf("Received - mid(%x) - clock_time(%d)\n", request->mid, clock_time());

  static vip_message_t rcv_pkt[1];
  if (vip_parse_common_header(rcv_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("vip_pkt have problem\n");
    return;
  }

  if (coap_get_query_variable(request, "start", &start))
  {
    rcv_pkt->start_time = atoi(start);
    printf("rcvd start time: %u\n", rcv_pkt->start_time);
  }

  if (coap_get_query_variable(request, "transmit", &transmit))
  {
    rcv_pkt->transmit_time = atoi(transmit);
    printf("rcvd transmit time: %u\n", rcv_pkt->transmit_time);
  }

  vip_route(rcv_pkt, &vr_type_handler);
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  /* check handover and loss */
  if(aa_id != rcv_pkt->aa_id || vt_id != rcv_pkt->vt_id) {
    ho_init_time = clock_time();
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

    vip_push_snd_buf(snd_pkt);
    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
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

    /* send vrc */
    vip_init_message(snd_pkt, VIP_TYPE_VRC, aa_id, vt_id, vr_id);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

    vip_push_snd_buf(snd_pkt);
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
  cumul_ack = rcv_pkt->vg_seq - 1;
  init_seq = rcv_pkt->vg_seq;

  /* send sec = sea's echo */
  vip_init_message(snd_pkt, VIP_TYPE_SEC, aa_id, vt_id, vr_id);
  vip_set_field_sec(snd_pkt, rcv_pkt->session_id, rcv_pkt->vg_seq);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

  vip_push_snd_buf(snd_pkt);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);

  /* trigger of vsd */
  timer_init(1);
}

static void
handler_sec(vip_message_t *rcv_pkt) {

}

static void
handler_vsd(vip_message_t *rcv_pkt) {
  if(!is_my_vip_pkt(rcv_pkt))
    return;

  retransmit_on();
  ctimer_restart(&ct);
  sliding_window_handler(rcv_pkt);

}

/* -------------------------- sliding window ---------------------------*/
void
sliding_window_handler(vip_message_t* rcv_pkt)
{
  // 먼저 패킷이 중복인지 체크
  int index = rcv_pkt->seq - init_seq;
  printf("index: %d rcvd_seq:%d init_seq:%d\n", index, rcv_pkt->seq, init_seq);
  if(simul_buffer[index] == 1)
  {
    // 중복 카운트
    if(++dup_cnt >= 2)
    {
      ack_flag = 1;
      dup_cnt = 0;
    }
  }
  else
  {
    simul_buffer[index] = 1;

    if(last_seq < rcv_pkt->seq)
      last_seq = rcv_pkt->seq;

    if(cumul_ack + 1 == rcv_pkt->seq)
    {
      // 기대했던 패킷이 왔음
      if(ho_init_time)
      {
        printf("--------------------------------> Handover Complete: %d\n", clock_time() - ho_init_time);
        ho_init_time = 0;
      }

      if (VIP_WINDOW_SIZE == 1)
      {
        // window size가 1일 경우, 기다리지않고 바로 ack를 보냄
        ack_flag = 1;
      }
      else
      {
        if (++consecutive_cnt >= 2)
        {
          ack_flag = 1;
          consecutive_cnt = 0;
        }
      }
      sliding_window_loss_search();
      printf("Cumul Ack:%d - last seq: %d\n", cumul_ack, last_seq);
    }
    else if(cumul_ack + 1 < rcv_pkt->seq)
    {
      // 기대한 것보다 더 크므로, loss = Out of Order
      printf("Out-of-Order\n");
      ack_flag = 1;
      out_of_order_flag = 1;
      sliding_window_loss_search();
      printf("Cumul Ack:%d - last seq: %d\n", cumul_ack, last_seq);
    }
    else
    {
      // 기대한 것보다 작음 = 중복 패킷임
       out_of_order_flag = 1;
    }
    
    if(ack_flag)
      sliding_window_send_ack();
  }
}

void
sliding_window_loss_search()
{
  int start = cumul_ack - init_seq + 1;
  int end = last_seq - init_seq;

  int j=0, chk=0;

  gap_num = 0;
  for(int i = start; i<=end; i++)
  {
    if(simul_buffer[i] == 1)
    {
      if(!chk)
        cumul_ack = i + init_seq;
    }
    else
    {
      gap_list[j++] = i + init_seq;
      chk = 1;
      gap_num++;
    }
  }

  // in-order이 되었다면 ack
  if(cumul_ack == last_seq && out_of_order_flag)
  {
    printf("In-Order\n");
    ack_flag = 1;
    out_of_order_flag = 0;
  }

  if(cumul_ack == init_seq + VIP_SIMUL_DATA-1 && !gap_num)
  {
    // 마지막이라면
    printf("--- Goal ---\n");
    retransmit_off();
    ack_flag = 1;
  }
}


void sliding_window_send_ack()
{
  vip_init_message(snd_pkt, VIP_TYPE_VDA, aa_id, vt_id, vr_id);
  vip_set_field_vda(snd_pkt, session_id, cumul_ack, gap_num, gap_list);
  vip_serialize_message(snd_pkt, buffer);
  vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

  vip_push_snd_buf(snd_pkt);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);

  ack_flag = 0;
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
  printf("TIMER ON\n");
  vip_timeout_swtich = 1;
}


void
retransmit_off()
{
  printf("TIMER OFF\n");
  vip_timeout_swtich = 0;
  simul_end = 1;
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

  vip_push_snd_buf(snd_pkt);
  process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
}

static void trigger_vsd(void* data)
{
    char payload[101];
    memset(payload, 1, 100);

    printf("- START Simulation -\n");
    printf("-- session(%x) ==> Goal vg_seq(%d) --\n", session_id, init_seq + VIP_SIMUL_DATA);

    vip_init_message(snd_pkt, VIP_TYPE_VSD, aa_id, vt_id, vr_id);
    vip_set_field_vsd(snd_pkt, session_id, vr_seq, (void *)payload, 100);
    vip_serialize_message(snd_pkt, buffer);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);

    vip_init_query(snd_pkt, query);
    vip_make_query_src(query, strlen(query), vr_id);
    vip_set_query(snd_pkt, query);

    snd_pkt->transmit_time = 0;

    vip_push_snd_buf(snd_pkt);
    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
    timer_init(2);
}

static void trigger_retransmit(void* data)
{
  if(vip_timeout_swtich && !simul_end)
  {
    printf("-----------------TIME OUT------------------\n");
    sliding_window_send_ack();
    ctimer_restart(&ct);
    ack_flag = 0;
  }
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
  case 2:
    ctimer_set(&ct, 3000, trigger_retransmit, NULL);
    break;
  }
}

static bool is_my_vip_pkt(vip_message_t* rcv_pkt)
{
  if(rcv_pkt->vr_id == vr_id)
    return true;
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
  list_remove(session_info, session);
  session->session_id = 0;
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
