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
static void handler_vu(vip_message_t *rcv_pkt);
static void handler_vm(vip_message_t *rcv_pkt);

static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char set_uri[50];
static int vr_id, aa_id, vt_id;

/* A simple actuator example. Toggles the red led */
RESOURCE(res_vr,
         "title=\"vr\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL);


/* vip type handler */
TYPE_HANDLER(vr_type_handler, handler_beacon, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, handler_vu, handler_vm, NULL);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Received\n");

  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
     printf("vip_pkt have problem\n");
     return;
  }
        
  /* check whether the rcv_pkt is beacon type or not */
  /* if not, ignore the rcv_pkt */
  if(vip_pkt->type == VIP_TYPE_BEACON || vip_pkt->vr_id == vr_id) {
    process_post(&vr_process, vr_rcv_event, (void *)vip_pkt);
  }
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  /* check handover */
  if(aa_id != rcv_pkt->aa_id || vt_id != rcv_pkt->vt_id) {
    printf("Received [%s] from aa(%d)\n", rcv_pkt->uplink_id, rcv_pkt->aa_id);

    /* update aa_id, vt_id */
    aa_id = rcv_pkt->aa_id;
    vt_id = rcv_pkt->vt_id;

    /* Recent Received vt-id, aa-id */
    vip_init_message(snd_pkt, VIP_TYPE_VRR, aa_id, vt_id);
    /* send to new aa */
    make_coap_uri(set_uri, aa_id);
    vip_set_dest_ep(snd_pkt, set_uri, VIP_AA_URL);

    /* set vr id to 0. it's mean not allocated*/
    vip_set_type_header_vr_id(snd_pkt, 0);

    vip_serialize_message(snd_pkt, buffer);

    process_post(&vr_process, vr_snd_event, (void *)snd_pkt);
  }
  else {
    printf("Ignore same beacon message..\n");
  }
}


static void
handler_vrr(vip_message_t *rcv_pkt) {
  vr_id = rcv_pkt->vr_id;
  printf("My ID is %d\n", vr_id);
}

static void
handler_vra(vip_message_t *rcv_pkt) {

}

static void
handler_vrc(vip_message_t *rcv_pkt) {

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
handler_vu(vip_message_t *rcv_pkt) {

}

static void
handler_vm(vip_message_t *rcv_pkt) {

}