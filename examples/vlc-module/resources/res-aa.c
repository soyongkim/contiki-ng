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
static char set_uri[50];

static int nonce_pool[65000];
static mutex_t p, e;

static char uplink_id[50] = {"ISL_AA_UPLINK_ID"};

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
  int nonce = 0;
  mutex_try_lock(&p);
  /* publish nonce_pool */
  for(int i=0; i<65000; i++) {
    if(!nonce_pool[i]) {
      nonce_pool[i] = 1;
      nonce = i;
      break;
    }
  }
  mutex_unlock(&p);
  return nonce;
}


int expire_nonce() {
  int nonce = 0;
  mutex_try_lock(&e);
  /* expire nonce_pool */
  for(int i=1; i<65000; i++) {
    if(nonce_pool[i]) {
      nonce_pool[i] = 0;
      nonce = i;
      break;
    }
  }
  mutex_unlock(&e);
  return nonce;
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

int
find_node_id(int vt_id) {
  vip_vt_tuple_t *c;
  for(c = list_head(vt_table); c != NULL; c = c->next) {
    if(c->vt_id == vt_id) {
      /* found vt id */
      return c->vt_id;
    }
  }

  /* not found */
  return 0;
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
  printf("Received\n");
  // printf("LEN:%d\n", request->payload_len);

  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("VIP: Not VIP Packet\n");
    return;
  }

  /* Publish nonce for vr */
  if(vip_pkt->type == VIP_TYPE_VRR && !vip_pkt->vr_id) {
    int nonce = publish_nonce();
    printf("pub:%d\n", nonce);
    printf("Request uri-host:%s\n", request->uri_host);
    printf("Request uri-path:%s\n", request->uri_path);

    printf("Response uri-host:%s\n", response->uri_host);
    printf("Respone uri-path:%s\n", response->uri_path);
    char res_payload[50];
    sprintf(res_payload, "%d", nonce);
    
    coap_set_status_code(response, CONTENT_2_05);
    coap_set_payload(response, res_payload, strlen(res_payload));
  }


  process_post(&aa_process, aa_rcv_event, (void *)vip_pkt);
}

static void
handler_beacon(vip_message_t *rcv_pkt) {
  printf("I'm beacon handler [%s]\n", rcv_pkt->uplink_id);
  
}


static void
handler_vrr(vip_message_t *rcv_pkt) {
  /* forward to vg */
  make_coap_uri(set_uri, VIP_VG_ID);
  vip_set_dest_ep(rcv_pkt, set_uri, VIP_VG_URL);

  printf("forward to vg(%d)\n", VIP_VG_ID);
  process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  int published_nonce = expire_nonce();
  /* forward to vt */
  make_coap_uri(set_uri, rcv_pkt->vt_id);
  vip_set_dest_ep(rcv_pkt, set_uri, VIP_VT_URL);
  vip_set_type_header_nonce(rcv_pkt, published_nonce);

  printf("forward to vt(%d)\n", rcv_pkt->vt_id);
  process_post(&aa_process, aa_snd_event, (void *)rcv_pkt);
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
  vip_init_message(snd_pkt, VIP_TYPE_ALLOW, node_id, rcv_pkt->vt_id);
  make_coap_uri(set_uri, rcv_pkt->vt_id);
  vip_set_dest_ep(snd_pkt, set_uri, VIP_VT_URL);

  vip_set_payload(snd_pkt, (void *)uplink_id, strlen(uplink_id));

  vip_serialize_message(snd_pkt, buffer);
  //printf("total? %d\n", snd_pkt->total_len);
  process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
}


static void
res_periodic_ad_handler(void)
{
  // vt 등록을 위한 첫 트랜잭션의 시작
  printf("Advertise...\n");

  /* pkt, type, aa-id(node_id), vt-id */
  vip_init_message(snd_pkt, VIP_TYPE_ALLOW, node_id, 0);
  vip_set_dest_ep(snd_pkt, VIP_BROADCAST_URI, VIP_VT_URL);

  vip_serialize_message(snd_pkt, buffer);
  process_post(&aa_process, aa_snd_event, (void *)snd_pkt);
}

