
#ifndef VIP
#define VIP
#include "net/ipv6/uip.h"

typedef struct vip_vt_tuple vip_vt_tuple_t;
typedef struct vip_nonce_tuple vip_nonce_tuple_t;
typedef struct session_s session_t;


/* message struct */
typedef struct {
    uint8_t *buffer;

    /* basis header */
    uint8_t type;
    uint32_t total_len;
    uint16_t aa_id;
    uint16_t vt_id;

    /* type specific header */
    char* uplink_id;
    uint32_t vr_id;
    uint32_t session_id;
    uint32_t vr_seq;
    uint32_t vg_seq;

    uint32_t payload_len;
    uint8_t *payload;

    /* for allocation vt */
    uint32_t nonce;

    /* for target handler */
    char *query, *dest_coap_addr, *dest_path;
    uint32_t query_len;
    uint32_t query_rcv_id;

    /* flag retransmit */
    uint32_t re_flag;
} vip_message_t;

struct vip_vt_tuple {
    vip_vt_tuple_t *next;
    int vt_id;
};

struct vip_nonce_tuple {
    vip_nonce_tuple_t *next;
    int nonce;
    int vr_node_id;
    int alloc_vr_id;
};

struct session_s {
    session_t* next;
    int vr_id;
    int vr_seq;
    int vg_seq;
    int session_id;
    uint8_t* data;
};


/* Interface for vip-pkt Serializaion */
void vip_init_message(vip_message_t *message, uint8_t type, uint16_t aa_id, uint16_t vt_id, uint32_t vr_id);
void vip_clear_message(vip_message_t *message);
int vip_serialize_message(vip_message_t *message, uint8_t *buffer);

/* for cooja, make uri using node id */
void vip_set_dest_ep_cooja(vip_message_t *message, char *dest_addr, int dest_node_id, char *dest_path);
void vip_set_non_flag(vip_message_t* message);

/* Set Type header field */
void vip_set_field_beacon(vip_message_t* message, char* uplink_id);

void vip_set_field_vrr(vip_message_t *message, int nonce);
void vip_set_field_vra(vip_message_t *message, int nonce);

void vip_set_field_ser(vip_message_t *message, int session_id, int vr_seq);
void vip_set_field_sea(vip_message_t *message, int session_id, int vg_seq);
void vip_set_field_sec(vip_message_t *message, int session_id, int vg_seq);

void vip_set_field_sdr(vip_message_t *message, int session_id, int vr_seq);
void vip_set_field_sda(vip_message_t *message, int session_id, int vg_seq);

void vip_set_field_alloc(vip_message_t *message, char* uplink_id);
void vip_set_payload(vip_message_t* message, void* payload, size_t payload_len);

/* coap's query function */
void vip_init_query(vip_message_t *message, char *query);
void vip_set_query(vip_message_t *message, char *query);
void vip_make_query_src(char* query, int query_len, int src_id);
void vip_make_query_nonce(char *query, int query_len, int value);
void vip_make_query_timer(char* query, int query_len, int flag);

/* Parse the vip-pkt on vip-interface.c */
int vip_parse_common_header(vip_message_t *message, uint8_t *data, uint16_t data_len);
void vip_parse_beacon(vip_message_t *message);

void vip_parse_vrr(vip_message_t *message);
void vip_parse_vra(vip_message_t *message);
void vip_parse_vrc(vip_message_t *message);
void vip_parse_rel(vip_message_t *message);

void vip_parse_ser(vip_message_t *message);
void vip_parse_sea(vip_message_t *message);
void vip_parse_sec(vip_message_t *message);

void vip_parse_sdr(vip_message_t *message);
void vip_parse_sda(vip_message_t *message);

void vip_payload_test(vip_message_t * message);



// /* Serialize Type Specific Header */
// int vip_serialize_beacon(vip_message_t *message);
// int vip_serialize_vrr(vip_message_t *message);
// int vip_serialize_vra(vip_message_t *message);
// int vip_serialize_vrc(vip_message_t *message);
// int vip_serialize_rel(vip_message_t *message);
// int vip_serialize_ser(vip_message_t *message);
// int vip_serialize_sea(vip_message_t *message);
// int vip_serialize_sec(vip_message_t *message);
// int vip_serialize_sd(vip_message_t *message);
// int vip_serialize_sda(vip_message_t *message);



#endif