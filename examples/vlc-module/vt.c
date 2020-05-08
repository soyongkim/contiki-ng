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
 *      Erbium (Er) CoAP Engine example.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aa.h"
#include "coap-engine.h"
#include "vip-interface.h"
#include "net/netstack.h"

/* Node ID */
#include "sys/node-id.h"
/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_vt;
extern vip_entity_t vt_type_handler;


/* test event process */
process_event_t vt_rcv_event, vt_snd_event;

int vt_id;
  
vip_message_t *rcv_pkt, *snd_pkt;


static void
aa_coap_request_handler(coap_message_t *res) {
  const uint8_t *chunk;

  if(res == NULL) {
    puts("Request timed out");
    return;
  }

  int len = coap_get_payload(res, &chunk);

  printf("Req-ack: %s\n", (char*)chunk);
}

static void
my_coap_request(vip_message_t *snd_pkt) {
  static coap_endpoint_t dest_ep;
  static coap_message_t request[1];

  coap_endpoint_parse(snd_pkt->dest_coap_addr, strlen(snd_pkt->dest_coap_addr), &dest_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_host(request, snd_pkt->dest_url);

  coap_set_payload(request, snd_pkt->buffer, snd_pkt->total_len);

  printf("-- VT Send coap vip[%d] packet --\n", snd_pkt->type);
  /* 일단 확실히 전송이 되는것부터 테스트 */
  COAP_BLOCKING_REQUEST(&dest_ep, request, aa_coap_request_handler);
}




PROCESS(vt_process, "VT");
AUTOSTART_PROCESSES(&vt_process);

PROCESS_THREAD(vt_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  printf("Node ID is %d\n", node_id);
  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  coap_activate_resource(&res_vt, "vip/vt");
  vt_id = 0;

  /* Define application-specific events here. */
  while(1) {
      PROCESS_WAIT_EVENT();

      if(ev == vt_rcv_event) {
        //res_vt.trigger();

        rcv_pkt = (vip_message_t *)data;
        printf("type is %d\n", rcv_pkt->type);

        // 여기서 route를 실행해야함 aa 프로세스가 route해서 보내야함
        vip_route(rcv_pkt, &vt_type_handler);
      }
      else if(ev == vt_snd_event) {
        snd_pkt = (vip_message_t *)data;
        my_coap_request(snd_pkt);
      }

      printf("EVENT!\n");
  }

  PROCESS_END();
}
