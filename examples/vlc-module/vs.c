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
 *      Erbium (Er) CoAP client example.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sdlib/vip/vip.h"

/* FIXME: This server address is hard-coded for Cooja and link-local for unconnected border router. */
/* default address */
// #define SERVER_EP "coap://[fe80::212:7402:0002:0202]"
/* test address */
#define SERVER_EP "coap://[fe80::201:1:1:1]"

/* Node ID */
#include "sys/node-id.h"


PROCESS(er_example_client, "Erbium Example Client");
AUTOSTART_PROCESSES(&er_example_client);

char *uplink_id = {"ISL-5GHz"};



/* This function is will be passed to COAP_BLOCKING_REQUEST() to handle responses. */
void client_chunk_handler(coap_message_t *response)
{
    const uint8_t *chunk;

    if (response == NULL)
    {
        puts("Request timed out");
        return;
    }

    int len = coap_get_payload(response, &chunk);

    printf("|%.*s", len, (char *)chunk);
}

PROCESS_THREAD(er_example_client, ev, data)
{
    static coap_endpoint_t server_ep;
    static struct etimer et;
    PROCESS_BEGIN();

    static coap_message_t request[1]; /* This way the packet can be treated as pointer as usual. */

    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

    static vip_message_t vip_pkt[1];
    uint8_t buffer[50];

    etimer_set(&et, CLOCK_SECOND);
    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        /* send a request to notify the end of the process */
        vip_init_message(vip_pkt, VIP_TYPE_BEACON, 1, 1);
        vip_set_type_header_uplink_id(vip_pkt, uplink_id);
        vip_set_header_total_len(vip_pkt, VIP_COMMON_HEADER_LEN + 8);
        vip_serialize_message(vip_pkt, buffer);

        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, "vip/aa");

        coap_set_payload(request, vip_pkt->buffer, vip_pkt->total_len);

        printf("--Requesting vip/aa--\n");

        COAP_BLOCKING_REQUEST(&server_ep, request,
                              client_chunk_handler);

        printf("\n--Done--\n");
        etimer_reset(&et);
    }

    PROCESS_END();
}
