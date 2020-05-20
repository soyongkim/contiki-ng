#include "contiki.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "lib/list.h"
#include "vg.h"
#include "cooja_addr.h"
#include "os/sys/mutex.h"

/* Node ID */
#include "sys/node-id.h"

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
static void handler_sd(vip_message_t *rcv_pkt);
static void handler_sda(vip_message_t *rcv_pkt);

static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char src_addr[50], dest_addr[50];


static mutex_t m;

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
              handler_sd, handler_sda, NULL);



int
find_new_vr_id() {
  for(int i=1; i<65000; i++) {
    if(!vr_id_pool[i]) {
      vr_id_pool[i] = 1;
      return i;
    }
  }

  return 0;
}



void
add_session_info(int vr_id, int session_id, int vg_seq, int vr_seq) {

}

/* if vr request new session, update last session info */
void
save_session_info(int vr_id, int session_id, int vg_seq, int vr_seq, uint8_t *data) {

}


void
allocate_vr_id(vip_message_t *rcv_pkt) {
    mutex_try_lock(&m);
    vip_init_message(snd_pkt, VIP_TYPE_VRA, rcv_pkt->aa_id, rcv_pkt->vt_id, find_new_vr_id());

    /* for vra pkt */
    vip_set_type_header_nonce(snd_pkt, 0);

    vip_set_ep_cooja(snd_pkt, src_addr, node_id, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);

    vip_serialize_message(snd_pkt, buffer);
    process_post(&vg_process, vg_snd_event, (void *)snd_pkt);
    mutex_unlock(&m);
}

void
handover_vr(int vr_id) {

}

/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Received\n");

  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("VIP: Not VIP Packet\n");
    return;
  }

  process_post(&vg_process, vg_rcv_event, (void *)vip_pkt);
}

static void 
res_event_handler(void) {
  printf("Init VG..\n");
}



static void
handler_vrr(vip_message_t *rcv_pkt) {
    /* case that vr is not allocated */
    if(!rcv_pkt->vr_id) {
      /* process concurrent reqeust problem from VRs */
      allocate_vr_id(rcv_pkt);
    }
    else {
        /* case handover */
        
    }
}

static void
handler_vra(vip_message_t *rcv_pkt) {


}

static void
handler_vrc(vip_message_t *rcv_pkt) {
  /* Allocate the vr-id */
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