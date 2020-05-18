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

static vip_message_t snd_pkt[1];
static uint8_t buffer[50];
static char set_uri[50];
static int vr_id, aa_id, vt_id;
static int service_num;
static int vr_id_pool;
static char* input_service[50];

LIST(vg_service_table);
LIST(vr_state_table);

/* A simple actuator example. Toggles the red led */
RESOURCE(res_vg,
         "title=\"VG\";rt=\"Control\"",
         NULL,
         res_post_handler,
         NULL,
         NULL);


/* vip type handler */
TYPE_HANDLER(vg_type_handler, NULL, handler_vrr, handler_vra, 
              handler_vrc, handler_rel, handler_ser, handler_sea, handler_sec,
              handler_sd, handler_sda, handler_vu, handler_vm, NULL);

/* check service num */
void
add_vr_tuple(int vr_id, int last_aa_id, int last_vt_id) {
  vip_vr_vg_tuple_t *new_tuple = malloc(sizeof(vip_vr_vg_tuple_t));
  int *cur_service_list = malloc(sizeof(int)*5000);
  int *vg_seq_list = malloc(sizeof(int)*5000);

  new_tuple->vr_id = vr_id;
  new_tuple->last_aa_id = last_aa_id;
  new_tuple->last_vt_id = last_vt_id;
  new_tuple->current_service_list = cur_service_list;
  new_tuple->vg_seq_list = vg_seq_list;

  list_add(vr_state_table, new_tuple);
}

void
remove_vr_tuple(vip_vr_vg_tuple_t* tuple) {
  list_remove(vr_state_table, tuple);
  free(tuple->current_service_list);
  free(tuple->vg_seq_list);
  free(tuple);
}

void
add_service_list(char *service_id) {
    vip_service_tuple_t *new_tuple = malloc(sizeof(vip_service_tuple_t));
    new_tuple->service_index = ++service_num;
    new_tuple->service_id = service_id;
    list_add(vg_service_table, new_tuple);
}

void
remove_service_list(vip_service_tuple_t *tuple) {
    list_remove(vr_state_table, tuple);
}


void
init_service() {
    input_service[0] = "First_Service";
    input_service[1] = "Second_Service";
    input_service[2] = "Third_Service";
    input_service[3] = "Forth_Service";
    input_service[4] = "Fifth_Service";

    for(int i=0; i<5; i++) {
      add_service_list(input_service[i]);
    }
}

void
allocate_vr_id(vip_message_t *rcv_pkt) {
    mutex_t m;
    mutex_try_lock(&m);
    vr_id_pool++;
    /* for vra pkt */
    snd_pkt->vr_id = vr_id_pool;
    /* add new vr tuple */
    add_vr_tuple(vr_id_pool, rcv_pkt->aa_id, rcv_pkt->vt_id);
    printf("vr_id_pool:%d\n", vr_id_pool);
    mutex_unlock(&m);
}

int
handover_vr(int vr_id, int last_aa_id, int last_vt_id) {
    int is_aa_change = 0;
    vip_vr_vg_tuple_t *c;
    for(c = list_head(vr_state_table); c != NULL; c = c->next) {
        if(c->vr_id == vr_id) {
            print("Handover aa[%d]vt[%d] -> aa[%d]vt[%d]\n", c->last_aa_id, c->last_vt_id, last_aa_id, last_vt_id);
            if(c->last_aa_id != aa_id) {
                is_aa_change = 1;
                c->last_aa_id = aa_id;
            }
            c->last_vt_id = vt_id;
            break;
        }
    }

    return is_aa_change;
}

/* called by coap-engine proc */
static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Received\n");

  static vip_message_t vip_pkt[1];
  if (vip_parse_common_header(vip_pkt, request->payload, request->payload_len) != VIP_NO_ERROR)
  {
    printf("VIP: Not VIP Packet\n");
    return;
  }

  process_post(&vg_process, vg_rcv_event, (void *)vip_pkt);
}

static void
handler_vrr(vip_message_t *rcv_pkt) {
    /* case that vr is not allocated */
    if(!rcv_pkt->vr_id) {
        vip_init_message(snd_pkt, VIP_TYPE_VRA, rcv_pkt->aa_id, rcv_pkt->vt_id);
        /* process concurrent reqeust problem from VRs */
        allocate_vr_id(rcv_pkt);

        make_coap_uri(set_uri, rcv_pkt->aa_id);
        vip_set_dest_ep(snd_pkt, set_uri, VIP_AA_URL);
        vip_serialize_message(snd_pkt, buffer);

        process_post(&vg_process, vg_snd_event, (void *)snd_pkt);
    }
    else {
        /* case handover */
        /* case aa handover */
        if(handover_vr(rcv_pkt->vr_id, rcv_pkt->aa_id, rcv_pkt->vt_id)) {
            /* Send vu */
            printf("Send VU\n");
        }
    }
}

static void
handler_vra(vip_message_t *rcv_pkt) {
  vr_id = rcv_pkt->vr_id;
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