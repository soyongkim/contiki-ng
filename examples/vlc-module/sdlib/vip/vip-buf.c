#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vip-buf.h"

static vip_snd_buf_t vip_snd_buf[1];

vip_snd_node_t* make_vip_pkt_node(vip_message_t* vip_pkt)
{
    vip_snd_node_t* node = calloc(1, sizeof(vip_snd_node_t));

    node->next = NULL;
    node->vip_pkt = calloc(1, sizeof(vip_message_t));

    /* dest ep info */
    node->vip_pkt->dest_coap_addr = calloc(50, sizeof(char));
    node->vip_pkt->dest_path = calloc(50, sizeof(char));
    memcpy(node->vip_pkt->dest_coap_addr, vip_pkt->dest_coap_addr, strlen(vip_pkt->dest_coap_addr));
    memcpy(node->vip_pkt->dest_path, vip_pkt->dest_path, strlen(vip_pkt->dest_path));

    /* query */
    if(vip_pkt->query_len)
    {
        node->vip_pkt->query_len = vip_pkt->query_len;
        node->vip_pkt->query = calloc(50, sizeof(char));
        memcpy(node->vip_pkt->query, vip_pkt->query, vip_pkt->query_len);
    }

    /* uplink */
    if(vip_pkt->uplink_id)
    {
        int uplink_id_len = strlen(vip_pkt->uplink_id);
        node->vip_pkt->uplink_id = calloc(50, sizeof(char));
        memcpy(node->vip_pkt->uplink_id, vip_pkt->uplink_id, uplink_id_len);
    }
    
    // /* serialized buffer */
    // node->vip_pkt->total_len = vip_pkt->total_len;
    // node->vip_pkt->buffer = calloc(50, sizeof(uint8_t));
    // memcpy(node->vip_pkt->buffer, vip_pkt->buffer, vip_pkt->total_len);
    
    // /* con, non type flag */
    // node->vip_pkt->re_flag = vip_pkt->re_flag;

    return node;
}


void vip_push_snd_buf(vip_message_t* vip_pkt) {

    vip_snd_node_t* new = make_vip_pkt_node(vip_pkt);

    if(!vip_snd_buf->head)
    {
        printf("First Push\n");
        vip_snd_buf->head = new;
        vip_snd_buf->tail = new;        
    }
    else
    {
        printf("Normal Push\n");
        vip_snd_buf->tail->next = new;
        vip_snd_buf->tail = new;
    }
}


vip_message_t* vip_front_snd_buf()
{
    return vip_snd_buf->head->vip_pkt;
}

void vip_pop_snd_buf()
{
    if(!vip_snd_buf->head)
    {
        printf("snd-buf is empty\n");
        return;
    }

    vip_snd_node_t* rm = vip_snd_buf->head;
    vip_snd_buf->head = vip_snd_buf->head->next;

    free(rm->vip_pkt->buffer);
    free(rm->vip_pkt->dest_coap_addr);
    free(rm->vip_pkt->dest_path);

    if(rm->vip_pkt->query_len)
        free(rm->vip_pkt->query);

    if(strlen(rm->vip_pkt->uplink_id))
        free(rm->vip_pkt->uplink_id);

    free(rm->vip_pkt);
    free(rm);
}