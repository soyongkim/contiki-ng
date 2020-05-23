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
static char query[11] = { "?src=" };

static int vr_id, aa_id, vt_id, rcv_nonce;
static int vip_timeout_swtich, loss_count;


/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];



/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request(vip_message_t *snd_pkt);

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
  //process_post(&vr_process, vr_rcv_event, (void *)vip_pkt);
}

bool
is_my_pkt(int rcv_vr_id) {
  if(rcv_vr_id == vr_id)
    return true;
  return false;
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
    vip_set_dest_ep_cooja(snd_pkt,dest_addr, aa_id, VIP_AA_URL);
    vip_set_type_header_nonce(snd_pkt, 0);
    /* set vr id to 0. it's mean not allocated*/
    vip_serialize_message(snd_pkt, buffer);

    vip_init_query(query);
    vip_make_query_src(query, node_id);
    vip_set_query(snd_pkt, query);

    vip_request(snd_pkt);
  } else {
    loss_handler();
  }
}

static void
loss_handler() {
  loss_count++;
  /* if the vr received same beacon frame, retransmit the pkt */
  if(loss_count >= 3) {
    /* Send recently sent pkt */
    vip_request(snd_pkt);
    vip_timeout_swtich = 0;
  }
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  /* received vr-id */
  if(rcv_nonce == rcv_pkt->nonce)
  {
    vr_id = rcv_pkt->vr_id;
    printf("I'm allocated vr-id(%d)!\n", vr_id);

    /* send vrc */
    vip_init_message(snd_pkt, VIP_TYPE_VRC, aa_id, vt_id, vr_id);
    vip_serialize_message(snd_pkt, buffer);
    vip_request(snd_pkt);
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


}


static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  const char *src = NULL;
  coap_request_state_t *state = &res_callback_state->state;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
      printf("Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
      if(state->response->code < 100) {
        if(coap_get_query_variable(state->response, "nonce", &src)) {
          rcv_nonce = atoi(src);
          printf("Nonce: %d\n", rcv_nonce);
        }
        /* Ack means that the vr wait for vip-ack-message by vt */ 
        vip_timeout_swtich = 1;
      }
  }
}

static void
vip_request(vip_message_t *snd_pkt) {
  /* set vip endpoint */
  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
  coap_init_message(request, COAP_TYPE_NON, COAP_POST, 0);
  coap_set_header_uri_path(request, snd_pkt->dest_path);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  if(snd_pkt->query)
    coap_set_header_uri_query(request, snd_pkt->query);

  printf("Send from %s to %s\n", snd_pkt->query, snd_pkt->dest_coap_addr);
  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
