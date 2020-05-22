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
LIST(vr_nonce_table);

/* for snd-pkt */
static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char dest_addr[50];
static char query[11] = { "?src=" };

/* for ack */
static vip_message_t ack_pkt[1];


static int nonce_pool[65000];
static mutex_t p;

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
  mutex_try_lock(&p);
  /* publish nonce_pool */
  for(int i=1; i<65000; i++) {
    if(!nonce_pool[i]) {
      nonce_pool[i] = 1;
      return i;
    }
  }
  mutex_unlock(&p);
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

int
vip_add_nonce_table(int vr_node_id) {
  vip_nonce_tuple_t* new_tuple = malloc(sizeof(vip_nonce_tuple_t));
  new_tuple->nonce = publish_nonce();
  new_tuple->vr_node_id = vr_node_id;
  list_add(vr_nonce_table, new_tuple);

  return new_tuple->nonce;
}

void
vip_remove_nonce_table(vip_nonce_tuple_t* tuple) {
  list_remove(vr_nonce_table, tuple);
  free(tuple);
}

vip_nonce_tuple_t*
vip_check_nonce_table(int vr_node_id) {
  vip_nonce_tuple_t* c;
  for(c = list_head(vr_nonce_table); c != NULL; c = c->next) {
    if(c->vr_node_id == vr_node_id) {
        return c;
    }
  }
  return NULL;
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


  vip_route(rcv_pkt, &aa_type_handler);


  if(ack_pkt->total_len > 0) {
    //coap_set_payload(response, ack_pkt->buffer, ack_pkt->total_len);
    coap_set_header_uri_query(response, query);
    ack_pkt->total_len = 0;
  }


  /* where is best place for change process */
  //process_post(&aa_process, aa_rcv_event, (void *)rcv_pkt);
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  printf("I'm beacon handler [%s]\n", rcv_pkt->uplink_id);
}


static void
handler_vrr(vip_message_t *rcv_pkt) {
  vip_nonce_tuple_t *chk;
  int nonce;
  if (!(chk = vip_check_nonce_table(rcv_pkt->query_rcv_id)))
  {
    /* publish new nonce to the vr */
    nonce = vip_add_nonce_table(rcv_pkt->query_rcv_id);

    /* Send vrr to vg */
    printf("forward to vg..\n");
    vip_set_ep_cooja(rcv_pkt, query, node_id, dest_addr, VIP_VG_ID, VIP_VG_URL);
    vip_serialize_message(rcv_pkt, buffer);
    //vip_request(rcv_pkt);
  }
  else
  {
    nonce = chk->nonce;
  }

  /* Set payload for ack */
  printf("Setting Ack..\n");
  vip_init_message(ack_pkt, VIP_TYPE_VRR, rcv_pkt->aa_id, rcv_pkt->vt_id, rcv_pkt->vr_id);
  vip_set_type_header_nonce(ack_pkt, nonce);
  vip_serialize_message(ack_pkt, buffer);
  /* test */
  vip_set_ep_cooja(ack_pkt, query, nonce, dest_addr, rcv_pkt->vt_id, VIP_VR_URL);

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
  /* remove nonce tuple if vrc received */
  vip_remove_nonce_table(vip_check_nonce_table(rcv_pkt->query_rcv_id));
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
      printf("Ack:%d - mid(%x)\n", state->response->code, state->response->mid);
      if(state->response->code < 100) {
        
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
