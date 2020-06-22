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
static void handler_vda(vip_message_t *rcv_pkt);


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
static vip_message_t snd_pkt[1];
static uint8_t buffer[VIP_MAX_PKT_SIZE];
static char dest_addr[50];

void sliding_window_transfer(vip_message_t *rcv_pkt, session_t* cur);
void sliding_window_sack_handler(vip_message_t *rcv_pkt, session_t* cur);
void show_buffer_state(session_t* cur);

/* use ack for query */
static vip_message_t ack_pkt[1];

static mutex_t v;

/* vr session_array */
/* array index is "VR-ID" */
//static vip_vr_session_tuple_t session_arr[65000];
/* vr id pool */
static int vr_id_pool[65000];

/* init time for throughput */
static unsigned long init_time;
static int init_msec;
static int goal_msec;
static unsigned long prev_time;
static int ho_time;
static unsigned long time_table[100];

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
              handler_vsd, handler_vda, NULL);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *src = NULL;
  printf("Received - mid(%x) - clock_time(%d)\n", request->mid, clock_time());

  static vip_message_t rcv_pkt[1];
  if (vip_parse_common_header(rcv_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("VIP: Not VIP Packet\n");
    return;
  }

  if(coap_get_query_variable(request, "src", &src)) {
    rcv_pkt->query_rcv_id = atoi(src);
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
    session_t* cur;
    if((cur = check_session(rcv_pkt->vr_id)))
    {
      printf("handover vr(%d)! => send to aa(%d) - vt(%d) == Cur time: %d\n", rcv_pkt->vr_id, rcv_pkt->aa_id, rcv_pkt->vt_id, clock_time()-init_msec);
      ho_time = clock_seconds()-init_time;
      if(VIP_HANDOVER_SWITCH)
      {
        // HO가 일어나면, 마지막을 보냈던 데이터 재전송 => cumul_ack + 1 ~ last_sent_ack까지 재전송
        int start = (cur->last_rcvd_ack + 1) - cur->init_seq;
        int end = (cur->last_sent_seq) - cur->init_seq;
        printf("Proposed Scheme! => last_ack:%d start:%d\n", cur->last_rcvd_ack, start);
        for (int i = start; i <= end; i++)
        {
          char payload[100];
          vip_init_message(snd_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
          vip_set_field_vsd(snd_pkt, cur->session_id, cur->init_seq + i, payload, 100);
          vip_serialize_message(snd_pkt, buffer);
          vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);
          vip_push_snd_buf(snd_pkt);
        }
      }
      else
      {
        // window = 1일 경우, 개선안된 HO 시나리오 수행
        // 일부러 중복 데이터를 2번 보내서 재전송을 유도함
        printf("Existing Scheme => last_ack:%d\n", cur->last_rcvd_ack - 1);
        char payload[100];
        vip_init_message(ack_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
        vip_set_field_vsd(ack_pkt, cur->session_id, cur->last_rcvd_ack - 1, (void *)payload, 100);
        vip_serialize_message(ack_pkt, buffer);
        vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);
        vip_push_snd_buf(snd_pkt);
        vip_push_snd_buf(snd_pkt);
      }

      process_post(&vg_process, vg_snd_event, (void *)snd_pkt);
      show_buffer_state(cur);
    }
    // handover end
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
    /* vg's vsd */
    session_t *cur;
    if((cur = check_session(rcv_pkt->vr_id)))
    {
      /* Trigger for data transfer */
      init_time = clock_seconds();
      init_msec = clock_time();
      printf("===================> init_time:%d\n", init_time);
      sliding_window_transfer(rcv_pkt, cur);    
    }
}

static void
handler_vda(vip_message_t *rcv_pkt)
{
    /* vg's vsd */
    session_t *cur;
    if((cur = check_session(rcv_pkt->vr_id)))
    {
      /* sack handler */
      sliding_window_sack_handler(rcv_pkt, cur);
    }
}


/* ----------------- function of sliding window ---------------------*/
void
sliding_window_transfer(vip_message_t *rcv_pkt, session_t* cur)
{
  /* window 여유 분 만큼 전송을 하자 last_cumul_ack까지 잘받았다는 의미로 사용하기 때문에
     +1 해주어서 그 다음부터 진행 */
  int start = (cur->last_rcvd_ack + 1) - cur->init_seq;
  printf("last_ack:%d init_seq:%d start:%d\n", cur->last_rcvd_ack, cur->init_seq, start);
  for(int i = start; i < start + VIP_WINDOW_SIZE; i++)
  {
    // 마지막 데이터
    if(i >= VIP_SIMUL_DATA)
      break;

    // 보냈던 데이터를 제외하고 연속 전송
    if(cur->simul_buffer[i] == 0)
    {
      cur->simul_buffer[i] = 1;
      char payload[100];
      vip_init_message(snd_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
      vip_set_field_vsd(snd_pkt, cur->session_id, cur->init_seq + i, payload, 100);
      vip_serialize_message(snd_pkt, buffer);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);
      vip_push_snd_buf(snd_pkt);

      // 최신 데이터라면 갱신
      if(cur->last_sent_seq <= cur->init_seq + i)
      {
        cur->last_sent_seq = cur->init_seq + i;
      }
    }
  }
  process_post(&vg_process, vg_snd_event, (void *)snd_pkt);
  show_buffer_state(cur);
}

void
sliding_window_sack_handler(vip_message_t *rcv_pkt, session_t* cur)
{
  int index = rcv_pkt->ack_seq - cur->init_seq;
  printf("last_ack:%d rcvd_ack:%d index:%d\n", cur->last_rcvd_ack, rcv_pkt->ack_seq, index);
  if(index >= 0 && cur->simul_buffer[index] == 1)
  {
    printf("Cumulative Ack: %d\n", rcv_pkt->ack_seq);
    // index 이전 까지의 모든 데이터를 잘받았다고 표시
    int start = cur->last_rcvd_ack - cur->init_seq >= 0 ? cur->last_rcvd_ack - cur->init_seq : 0;
    for(int i = start; i <= index; i++)
    {
      cur->simul_buffer[i] = 2;
    }

    // last_ack이 옮겨졌으니 throuput 계산
    unsigned long time = (clock_seconds() - init_time) > 0 ? (clock_seconds() - init_time) : 1;
    if(time != prev_time)
    {
      int time_idx;
      if(prev_time < 100)
      {
        if(ho_time && prev_time >= ho_time)
        {
          time_idx = prev_time - (ho_time - 30); 
        }
        else
        {
          time_idx = prev_time;
        }
        
        time_table[time_idx] = (cur->last_rcvd_ack - cur->init_seq)+1;
      }
      printf("========================= Cur time:%d! => [Prev Processed Data: %d / Prev time: %d]\n", time, (cur->last_rcvd_ack - cur->init_seq)+1, time_idx);
      prev_time = time;
    }
    // index까지 잘받았으니 last_ack를 옮김. 만약 이게 안옮겨지면, 중복 ack를 받았다는 말임
    cur->last_rcvd_ack = cur->init_seq + index;

    // last data check
    if(rcv_pkt->ack_seq == cur->init_seq + VIP_SIMUL_DATA-1)
    {
      goal_msec = clock_time() - init_msec;
    }


    if(goal_msec)
    {
      printf("--------------------------------------GOAL---------------------------\n");
      printf("%d\n", goal_msec);
    }


    int prev_sec_data;
    if (prev_time >= 60)
    {
      printf("================================== Total Data =================================\n");
      for (int i = 1; i <= 60; i++)
      {
        if(time_table[i])
        {
          printf("[%d]sec\t%d\n", i, time_table[i]);
          prev_sec_data = time_table[i];
        }
        else
        {
          printf("[%d]sec\t%d\n", i, prev_sec_data);
        }
        
      }
      printf("================================== END ========================================\n");
    }

    if (prev_time >= 65)
    {
      printf("==> Handover Time: %d\n", ho_time);
      for (int i = 61; i <= 65; i++)
      {
        if (time_table[i])
        {
          printf("[%d]sec\t%d\n", i, time_table[i]);
          prev_sec_data = time_table[i];
        }
        else
        {
          printf("[%d]sec\t%d\n", i, prev_sec_data);
        }
      }
      printf("================================== HO ========================================\n");
    }

    // 그리고 여유분 전송
    sliding_window_transfer(rcv_pkt, cur);
  }
  else if(cur->simul_buffer[index] == 2)
  {
    // 중복 ack 체크
    cur->dup_ack = 1;
  }
 

  // 갭확인
  if(rcv_pkt->gap_len)
  {
    for(int i=0; i<rcv_pkt->gap_len; i++)
    {
      // gap 부분만 다시 보냄
      char payload[100];
      vip_init_message(snd_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
      // 해당 gap seq를 첨가하여 전송. 테스트니까 페이로드는 모두 같은 값임
      vip_set_field_vsd(snd_pkt, cur->session_id, rcv_pkt->gap_list[i], payload, 100);
      vip_serialize_message(snd_pkt, buffer);
      vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);
      vip_push_snd_buf(snd_pkt);
      process_post(&vg_process, vg_snd_event, (void *)snd_pkt);
    }
    free(rcv_pkt->gap_list);
  }
  else
  {
    // 갭이 없고 같은 중복 Ack를 받은 경우, Fast Retransmission
    // 또한 이 케이스는 Receiver가 받은 패킷까지는 갭이 없지만 Sender가 보낸 최신 데이터에 loss가 발생한 케이스를 해결하기 위한 솔루션이기도 함
    if (cur->dup_ack)
    {
      // 만약 중복 Ack이 원래 전송하고자하는 데이터의 마지막 Ack였다면, 재전송안함
      if(rcv_pkt->ack_seq == cur->init_seq + VIP_SIMUL_DATA -1)
        return;
      // 중복 Ack를 받았다면, cumul_ack + 1 ~ last_sent_ack까지 재전송
      int start = (cur->last_rcvd_ack + 1) - cur->init_seq;
      int end = (cur->last_sent_seq) - cur->init_seq;
      printf("Dup case! => last_ack:%d start:%d\n", cur->last_rcvd_ack, start);
      for (int i = start; i <= end; i++)
      {
        char payload[100];
        vip_init_message(snd_pkt, VIP_TYPE_VSD, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
        vip_set_field_vsd(snd_pkt, cur->session_id, cur->init_seq + i, payload, 100);
        vip_serialize_message(snd_pkt, buffer);
        vip_set_dest_ep_cooja(snd_pkt, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);
        vip_push_snd_buf(snd_pkt);
      }
      process_post(&vg_process, vg_snd_event, (void *)snd_pkt);
      show_buffer_state(cur);
      cur->dup_ack = 0;
    }
  }
}

void
show_buffer_state(session_t* cur)
{
  printf("cur-state\n");
  int start = 0;
  for(; start < VIP_SIMUL_DATA; start++)
  {
    printf("[%d] ", cur->simul_buffer[start]);
  }
  printf("\n");
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

    new->init_seq = vg_seq;
    new->last_rcvd_ack = vg_seq -1;
    new->last_sent_seq = 0;
    new->simul_buffer = calloc(VIP_SIMUL_DATA, sizeof(int));

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