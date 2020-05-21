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
#include "vt.h"
#include "cooja_addr.h"

/* Node ID */
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void beaconing(void);

static void handler_vrr(vip_message_t *rcv_pkt);
static void handler_vra(vip_message_t *rcv_pkt);
static void handler_vrc(vip_message_t *rcv_pkt);
static void handler_rel(vip_message_t *rcv_pkt);
static void handler_ser(vip_message_t *rcv_pkt);
static void handler_sea(vip_message_t *rcv_pkt);
static void handler_sec(vip_message_t *rcv_pkt);
static void handler_sd(vip_message_t *rcv_pkt);
static void handler_sda(vip_message_t *rcv_pkt);
static void request_vt_id_handler(vip_message_t *rcv_pkt);

static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static int vt_id, aa_id;
static char dest_addr[50];
static char query[11] = { "?src=" };

static char uplink_id[50];

/* A simple actuator example. Toggles the red led */
PERIODIC_RESOURCE(res_vt,
         "title=\"VT\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL,
         5000,
         beaconing);



/* vip type handler */
TYPE_HANDLER(vt_type_handler, NULL, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, request_vt_id_handler);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Received - %d\n", request->mid);
  
  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) == VIP_NO_ERROR)
  {
    printf("VIP: NO ERROR\n");
  }
  else
  {
    printf("VIP: Not VIP Packet\n");
  }
  process_post(&vt_process, vt_rcv_event, (void *)vip_pkt);
}

/* beaconing */
static void
beaconing() {
  /* if the vt is complete to register to aa, start beaconing */
  if(aa_id) {
    printf("Beaconing...\n");
    /* you have to comfirm that the type field is fulled */
    vip_init_message(snd_pkt, VIP_TYPE_BEACON, aa_id, vt_id, 0);
    vip_set_ep_cooja(snd_pkt, query, vt_id, dest_addr, 0, VIP_VR_URL);

    vip_set_type_header_uplink_id(snd_pkt, uplink_id);
    vip_serialize_message(snd_pkt, buffer);
    process_post(&vt_process, vt_snd_event, (void *)snd_pkt);
  }
}

static void
handler_vrr(vip_message_t *rcv_pkt) {
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  vip_set_dest_ep(rcv_pkt, VIP_BROADCAST_URI, VIP_VR_URL);
  process_post(&vt_process, vt_snd_event, (void *)rcv_pkt);
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
request_vt_id_handler(vip_message_t *rcv_pkt) {
  /* broadcast ad pkt */
  if(!rcv_pkt->vt_id) {
    /* if vt was already registered, ignore ad pkt */
    if(vt_id)
      return;

    /* send ack pkt for ad pkt */
    vip_init_message(snd_pkt, VIP_TYPE_ALLOW, rcv_pkt->aa_id, node_id, 0);
    vip_set_ep_cooja(snd_pkt, query, node_id, dest_addr, rcv_pkt->aa_id, VIP_AA_URL);
    vip_serialize_message(snd_pkt, buffer);

    process_post(&vt_process, vt_snd_event, (void *)snd_pkt);
  }
  else {
    if(!vt_id) {
      vt_id = rcv_pkt->vt_id;
      aa_id = rcv_pkt->aa_id;
      strcpy(uplink_id, rcv_pkt->uplink_id);
      printf("vt-[%d] is allocated by aa-[%d]\n", vt_id, aa_id);
      printf("uplink-id is [%s]\n", uplink_id);
    } else {
      printf("Already allocated with %d\n", vt_id);
    }
  }
}