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


LIST(vr_nonce_table);

/* for snd-pkt */
static uint8_t buffer[50];

/* use ack for query */
static vip_message_t ack_pkt[1];

static mutex_t v;



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
add_nonce_table(int nonce) {
  vip_nonce_tuple_t* new_tuple = malloc(sizeof(vip_nonce_tuple_t));
  new_tuple->alloc_vr_id = publish_vrid();
  new_tuple->nonce = nonce;
  list_add(vr_nonce_table, new_tuple);

  return new_tuple->alloc_vr_id;
}

void
remove_nonce_table(vip_nonce_tuple_t* tuple) {
  list_remove(vr_nonce_table, tuple);
  free(tuple);
}

vip_nonce_tuple_t*
check_nonce_table(int vr_node_id) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_nonce_table); c != NULL; c = c->next) {
    if(c->vr_node_id == vr_node_id) {
        return c;
    }
  }
  return NULL;
}


void
add_session_info(int vr_id, int session_id, int vg_seq, int vr_seq) {

}

/* if vr request new session, update last session info */
void
save_session_info(int vr_id, int session_id, int vg_seq, int vr_seq, uint8_t *data) {

}


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *src = NULL;
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

  vip_route(rcv_pkt, &vg_type_handler);

  /* for ack */
  if(ack_pkt->total_len)
    coap_set_payload(response, ack_pkt->buffer, ack_pkt->total_len);
  if(ack_pkt->query_len)
    coap_set_header_uri_query(response, ack_pkt->query);
}

static void 
res_event_handler(void) {
  printf("Init VG..\n");
}



static void
handler_vrr(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  int alloc_vr_id;
  if (!(chk = check_nonce_table(rcv_pkt->nonce)))
  {
    /* publish new vr-id */
    alloc_vr_id = add_nonce_table(rcv_pkt->nonce);
  }
  else
  {
    alloc_vr_id = chk->alloc_vr_id;
  }

  /* Set payload for ack */
  printf("Setting Ack..\n");
  vip_init_message(ack_pkt, VIP_TYPE_VRA, rcv_pkt->aa_id, rcv_pkt->vt_id, alloc_vr_id);
  vip_set_type_header_nonce(ack_pkt, rcv_pkt->nonce);
  vip_serialize_message(ack_pkt, buffer);
}

static void
handler_vra(vip_message_t *rcv_pkt) {


}

static void
handler_vrc(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  /* if vrc is duplicated, the tuple is null. so nothing to do and just send ack */
  if (!(chk = check_nonce_table(rcv_pkt->vr_id)))
  {
    /* remove nonce tuple if vrc received */
    //remove_nonce_table(chk);

    printf("vr[%d] complete!\n", rcv_pkt->vr_id);
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