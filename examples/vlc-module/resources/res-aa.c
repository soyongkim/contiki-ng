/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"
#include "coap-engine.h"
#include "coap-callback-api.h"
#include "vip-interface.h"
#include "lib/list.h"
#include "aa.h"
#include "cooja_addr.h"
#include "os/sys/mutex.h"

/* Node ID */
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_ad_handler(void);

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
static void allocate_vt_handler(vip_message_t *rcv_pkt);

/* make vt table which administrate the vt id */
LIST(vt_table);

/* for snd-pkt */
static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char dest_addr[50];
static char query[11] = { "?src=" };

static int nonce_pool[65000];
//static mutex_t p, e;

static char uplink_id[50] = {"ISL_AA_UPLINK_ID"};

/* for send packet */
static coap_callback_request_state_t callback_state;
static coap_endpoint_t dest_ep;
static coap_message_t request[1];



/* using coap callback api */
static void vip_request_callback(coap_callback_request_state_t *callback_state);
static void vip_request(vip_message_t *snd_pkt);



/* A simple actuator example. Toggles the red led */
PERIODIC_RESOURCE(res_aa,
         "title=\"AA\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL,
         30000,
         res_periodic_ad_handler);


/* vip type handler */
TYPE_HANDLER(aa_type_handler, handler_beacon, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, allocate_vt_handler);


int
publish_nonce() {
  /* publish nonce_pool */
  for(int i=1; i<65000; i++) {
    if(!nonce_pool[i]) {
      nonce_pool[i] = 1;
      return i;
    }
  }
  return 0;
}

int 
expire_nonce() {
  int nonce = 0;
  //mutex_try_lock(&e);
  /* expire nonce_pool */
  for(int i=1; i<65000; i++) {
    if(nonce_pool[i]) {
      nonce_pool[i] = 0;
      nonce = i;
      break;
    }
  }
  //mutex_unlock(&e);
  return nonce;
}

void
allocation_vr(vip_message_t* rcv_pkt) {
  //mutex_try_lock(&p);
  int nonce = publish_nonce();
  printf("[Pub]: %d\n", nonce);
  /* Send nonce to vr using vrr. It's possble that vr doesn't receive vrr type in main flow */
  /* use vr_id field to send the nonce */
  vip_init_message(snd_pkt, VIP_TYPE_VRR, node_id, 0, nonce);
  vip_set_ep_cooja(snd_pkt, query, node_id, dest_addr, rcv_pkt->query_rcv_id, VIP_VR_URL);
  vip_serialize_message(snd_pkt, buffer);
  vip_request(snd_pkt);
  //mutex_unlock(&p);
}

void
handover_vr(vip_message_t* rcv_pkt) {
  /* forward to vg */
  vip_set_ep_cooja(rcv_pkt, query, node_id, dest_addr, VIP_VG_ID, VIP_VT_URL);
  printf("forward to vg(%d)\n", VIP_VG_ID);
  vip_request(rcv_pkt);
}

void
add_vt_id_tuple(int node_id) {
  vip_vt_tuple_t *new_tuple = malloc(sizeof(vip_vt_tuple_t));
  new_tuple->vt_id = node_id;
  list_add(vt_table, new_tuple);
}

void
remove_vt_id_tuple(vip_vt_tuple_t* tuple) {
  list_remove(vt_table, tuple);
  free(tuple);
}


void show_vt_table() {
  vip_vt_tuple_t *c;
  for(c = list_head(vt_table); c != NULL; c = c->next) {
    printf("[aa(%d) -> vt(%d):]\n", node_id, c->vt_id);
  }
}

/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *src = NULL;
  printf("Received - %d\n", request->mid);

  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("VIP: Not VIP Packet\n");
    return;
  }


  if(coap_get_query_variable(request, "src", &src)) {
    vip_pkt->query_rcv_id = atoi(src);
  }
  
  process_post(&aa_process, aa_rcv_event, (void *)vip_pkt);
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  printf("I'm beacon handler [%s]\n", rcv_pkt->uplink_id);
}


static void
handler_vrr(vip_message_t *rcv_pkt) {
  if(!rcv_pkt->vr_id) {
      allocation_vr(rcv_pkt);
  }
  else {
      handover_vr(rcv_pkt);
  }
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  int published_nonce = expire_nonce();
  /* forward to vt */
  vip_set_ep_cooja(rcv_pkt, query, node_id, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);
  vip_set_type_header_nonce(rcv_pkt, published_nonce);

  printf("forward to vt(%d)\n", rcv_pkt->vt_id);
  vip_request(snd_pkt);
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
allocate_vt_handler(vip_message_t *rcv_pkt) {
  /* rcv_pkt->vt_id is the vt's "NODE ID" */
  add_vt_id_tuple(rcv_pkt->vt_id);
  show_vt_table();

  /* pkt, type, aa-id(node_id), vt-id(target vt's node id) */
  vip_init_message(snd_pkt, VIP_TYPE_ALLOW, node_id, rcv_pkt->vt_id, 0);
  vip_set_ep_cooja(snd_pkt, query, node_id, dest_addr, rcv_pkt->vt_id, VIP_VT_URL);

  vip_set_payload(snd_pkt, (void *)uplink_id, strlen(uplink_id));

  vip_serialize_message(snd_pkt, buffer);
  vip_request(snd_pkt);
}


static void
res_periodic_ad_handler(void)
{
  // vt 등록을 위한 첫 트랜잭션의 시작
  printf("Advertise...\n");

  /* pkt, type, aa-id(node_id), vt-id */
  vip_init_message(snd_pkt, VIP_TYPE_ALLOW, node_id, 0, 0);
  vip_set_ep_cooja(snd_pkt, query, node_id, dest_addr, 0, VIP_VT_URL);
  vip_serialize_message(snd_pkt, buffer);
  vip_request(snd_pkt);
}


static void
vip_request_callback(coap_callback_request_state_t *res_callback_state) {
  coap_request_state_t *state = &res_callback_state->state;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
      printf("CODE:%d\n", state->response->code);
      if(state->response->code > 100) {
          //printf("4.xx -> So.. try to retransmit\n");
          //coap_send_request(&callback_state, &dest_ep, state->request, vip_request_callback);
      }
  }
}

static void
vip_request(vip_message_t *snd_pkt) {
  /* set vip endpoint */
  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(request, snd_pkt->dest_path);
  coap_set_header_uri_query(request, snd_pkt->query);
  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  printf("Send from %s to %s\n", snd_pkt->query, snd_pkt->dest_coap_addr);
  coap_send_request(&callback_state, &dest_ep, request, vip_request_callback);
}
