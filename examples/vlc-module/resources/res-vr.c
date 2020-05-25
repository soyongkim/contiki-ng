#include "contiki.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "vr.h"
#include "cooja_addr.h"

/* Node ID */
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

static void handler_beacon(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_sd(vip_message_t *rcv_pkt);
static void handler_sda(vip_message_t *rcv_pkt);

static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char dest_addr[50];
static char query[50];

static int vr_id, aa_id, vt_id;
static int vip_timeout_swtich;
static int loss_count = 0;


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
              NULL, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, NULL);


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
    /* send to new aa */
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, aa_id, VIP_AA_URL);
    vip_set_type_header_nonce(snd_pkt, 0);
    /* set vr id to 0. it's mean not allocated*/
    vip_serialize_message(snd_pkt, buffer);

    vip_init_query(query);
    vip_make_query_src(query, strlen(query), node_id);
    vip_set_query(snd_pkt, query);

    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
  } else {
    if(vip_timeout_swtich) {
      loss_handler();
    }
  }
}

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
    printf("loss count\n");
    snd_pkt->re_flag = 1;
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
    retransmit_off();

    /* send vrc */
    // vip_init_message(snd_pkt, VIP_TYPE_VRC, aa_id, vt_id, vr_id);
    // vip_serialize_message(snd_pkt, buffer);
    // vip_request(snd_pkt);
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
res_event_handler(void) {
  /* retransmit switch */
  retransmit_on();
}