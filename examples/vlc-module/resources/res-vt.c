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
static void handler_vsd(vip_message_t *rcv_pkt);
static void handler_alloc(vip_message_t *rcv_pkt);

static int vt_id, aa_id;

/* for snd-pkt */
static vip_message_t snd_pkt[1];
static uint8_t buffer[VIP_MAX_PKT_SIZE];
static char dest_addr[50];
static char query[VIP_MAX_QUERY_SIZE];

/* use ack for query */
static vip_message_t ack_pkt[1];

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
              handler_vsd, handler_alloc);


/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{

  const char *start = NULL;
  const char *transmit = NULL;
  printf("Received - mid(%x) - clock_time(%d)\n", request->mid, clock_time());
  
  static vip_message_t rcv_pkt[1];
  if (vip_parse_common_header(rcv_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("vip_pkt have problem\n");
    return;
  }

  if (coap_get_query_variable(request, "start", &start))
  {
    rcv_pkt->start_time = atoi(start);
    printf("rcvd start time: %d\n", rcv_pkt->start_time);
  }

  if (coap_get_query_variable(request, "transmit", &transmit))
  {
    rcv_pkt->transmit_time = atoi(transmit);
    printf("rcvd transmit time: %d\n", rcv_pkt->transmit_time);
  }

  vip_route(rcv_pkt, &vt_type_handler);

  /* for ack */
  coap_set_status_code(response, CONTENT_2_05);
  if(ack_pkt->total_len)
  {
    coap_set_payload(response, ack_pkt->buffer, ack_pkt->total_len);
    ack_pkt->total_len = 0;
  }
  if(ack_pkt->query_len)
  {
    coap_set_header_uri_query(response, ack_pkt->query);
    ack_pkt->query_len = 0;
  }
}

/* beaconing */
static void
beaconing() {
  /* if the vt is complete to register to aa, start beaconing */
  if(aa_id) {
    printf("Beaconing...\n");
    /* you have to comfirm that the type field is fulled */
    vip_init_message(snd_pkt, VIP_TYPE_BEACON, aa_id, vt_id, 0);
    vip_set_dest_ep_cooja(snd_pkt, dest_addr, VIP_BROADCAST, VIP_VR_URL);
    vip_set_field_beacon(snd_pkt, uplink_id);
    vip_serialize_message(snd_pkt, buffer);
    process_post(&vt_process, vt_snd_event, (void *)snd_pkt);
  }
}

static void
handler_vrr(vip_message_t *rcv_pkt) {
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  /* VLC! */
  printf("Broadcast VRA for VR(%d)\n", rcv_pkt->vr_id);
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_BROADCAST, VIP_VR_URL);
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
  /* VLC! */
  printf("Broadcast SEA for VR(%d) - vg_seq(%d)\n", rcv_pkt->vr_id, rcv_pkt->vg_seq);
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_BROADCAST, VIP_VR_URL);
  process_post(&vt_process, vt_snd_event, (void *)rcv_pkt);
}

static void
handler_sec(vip_message_t *rcv_pkt) {

}

static void
handler_vsd(vip_message_t *rcv_pkt)
{
  if (rcv_pkt->start_time)
  {
    uint32_t cur_time = RTIMER_NOW() / 1000;
    printf("Cur time: %u\n", cur_time);
    rcv_pkt->transmit_time += cur_time - rcv_pkt->start_time;
    printf("transmit time: %u\n", rcv_pkt->transmit_time);
  }

  vip_init_query(rcv_pkt, query);
  vip_make_query_transmit_time(query, strlen(query), (uint32_t)rcv_pkt->transmit_time);
  vip_set_query(rcv_pkt, query);

  /* VLC! */
  printf("Broadcast VSD for VR(%d) <= seq(%d)\n", rcv_pkt->vr_id, rcv_pkt->seq);
  vip_set_dest_ep_cooja(rcv_pkt, dest_addr, VIP_BROADCAST, VIP_VR_URL);
  process_post(&vt_process, vt_snd_event, (void *)rcv_pkt);
}

static void
handler_alloc(vip_message_t *rcv_pkt) {
  /* broadcast ad pkt */
  if(!vt_id) {
    vt_id = node_id;
    aa_id = rcv_pkt->aa_id;
    strcpy(uplink_id, rcv_pkt->uplink_id);
    printf("vt(%d) - aa(%d) - [%s]\n", vt_id, aa_id, uplink_id);
  }
  else {
    printf("Already allocated with %d\n", vt_id);
  }
} 