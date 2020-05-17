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
static void beconing(void);

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
static void request_vt_id_handler(vip_message_t *rcv_pkt);

static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char set_uri[50];
static int vt_id, aa_id;

/* A simple actuator example. Toggles the red led */
PERIODIC_RESOURCE(res_vt,
         "title=\"VT\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL,
         5000,
         beconing);


/* vip type handler */
TYPE_HANDLER(vt_type_handler, handler_beacon, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, handler_vu, handler_vm, request_vt_id_handler);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Received\n");
  printf("LEN:%d\n", request->payload_len);

  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) == VIP_NO_ERROR)
  {
    printf("VIP: NO ERROR\n");
  }
  else
  {
    printf("VIP: Not VIP Packet\n");
  }

  printf("aa-id:%d | vt-id:%d\n", vip_pkt->aa_id, vip_pkt->vt_id);
  process_post(&vt_process, vt_rcv_event, (void *)vip_pkt);
}

/* handler for beacon type pkt */
static void
handler_beacon(vip_message_t *rcv_pkt) {
  printf("I'm beacon handler [%s]\n", rcv_pkt->uplink_id);
  
}

/* beaconing */
static void
beconing() {
  /* if the vt is complete to register to aa, start beaconing */
  if(aa_id) {
    printf("Beaconing...\n");

    vip_init_message(snd_pkt, VIP_TYPE_BEACON, aa_id, vt_id);
    make_coap_uri(set_uri, node_id);
    vip_set_dest_ep(snd_pkt, set_uri, VIP_VR_URL);
    vip_serialize_message(snd_pkt, buffer);
    //process_post(&vt_process, vt_snd_event, (void *)snd_pkt);
  }
}



static void
handler_vrr(vip_message_t *rcv_pkt) {

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

static void
request_vt_id_handler(vip_message_t *rcv_pkt) {
  if(!vt_id && !rcv_pkt->vt_id) {
    /* pkt, type, aa-id, vt-id(my node id) */
    aa_id = rcv_pkt->aa_id;
    // vip_init_message(snd_pkt, VIP_TYPE_ALLOW, rcv_pkt->aa_id, node_id);
    // vip_set_dest_ep(snd_pkt, VIP_BROADCAST_URI, VIP_AA_URL);
    // vip_serialize_message(snd_pkt, buffer);
    // rocess_post(&vt_process, vt_snd_event, (void *)snd_pkt);
  }
  else {
    if(!vt_id) {
      vt_id = node_id;
      printf("[%d] is allocated\n", vt_id);
    } else {
      printf("Already allocated with %d\n", vt_id);
    }
  }
}