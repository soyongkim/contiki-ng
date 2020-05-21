#include "net/ipv6/uip.h"
#include "vip-constants.h"

typedef struct vip_vt_tuple vip_vt_tuple_t;
typedef struct vip_vr_session_tuple vip_vr_session_tuple_t;

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
    uint32_t vr_seq_number;
    uint32_t vg_seq_number;

    uint32_t payload_len;
    uint8_t *payload;

    /* for allocation vt */
    uint32_t nonce;

    /* for target handler */
    char *query, *dest_coap_addr, *dest_path;
    uint32_t query_rcv_id;
} vip_message_t;


struct vip_vt_tuple {
    vip_vt_tuple_t *next;
    int vt_id;
};


typedef struct vip_session_info {
    int session_id;
    int vg_seq;
    int vr_seq;
    uint8_t *recent_vg_data;
} vip_session_info_t;


struct vip_vr_session_tuple {
    vip_session_info_t** session_info;
    vip_session_info_t* recent_session_info;
};


/* Interface for vip-pkt Serializaion */
void vip_init_message(vip_message_t *message, uint8_t type, uint16_t aa_id, uint16_t vt_id, uint32_t vr_id);
int vip_serialize_message(vip_message_t *message, uint8_t *buffer);
int vip_set_dest_ep(vip_message_t *message, char *dest_addr, char *dest_url);

/* for cooja, make uri from node id */
void vip_set_ep_cooja(vip_message_t *message, char* src_addr, int src_id, char* dest_addr, int dest_id, char *path);

/* Parse the vip-pkt on vip-interface.c */
int vip_parse_common_header(vip_message_t *message, uint8_t *data, uint16_t data_len);
uint32_t vip_parse_int_option(uint8_t *bytes, size_t length);
void vip_parse_beacon(vip_message_t *message);
void vip_parse_VRR(vip_message_t *message);
void vip_parse_VRA(vip_message_t *message);
void vip_parse_VRC(vip_message_t *message);
void vip_parse_REL(vip_message_t *message);
void vip_parse_SER(vip_message_t *message);
void vip_parse_SEA(vip_message_t *message);
void vip_parse_SEC(vip_message_t *message);
void vip_parse_SD(vip_message_t *message);
void vip_parse_SDA(vip_message_t *message);
void vip_payload_test(vip_message_t * message);


/* Data configure */
int vip_get_header_total_len(vip_message_t *message, uint32_t *total_len);
int vip_set_header_total_len(vip_message_t *message, uint32_t total_len);

int vip_get_header_aa_id(vip_message_t *message, uint16_t *aa_id);
int vip_set_header_aa_id(vip_message_t *message, uint16_t aa_id);

int vip_get_header_vt_id(vip_message_t *message, uint16_t *vt_id);
int vip_set_header_vt_id(vip_message_t *message, uint16_t vt_id);

int vip_get_type_header_uplink_id(vip_message_t *message, char **uplink_id);
int vip_set_type_header_uplink_id(vip_message_t *message, char *uplink_id); 

int vip_get_type_header_vr_id(vip_message_t *message, uint32_t *vr_id);
int vip_set_type_header_vr_id(vip_message_t *message, uint32_t vr_id);
int vip_set_type_header_nonce(vip_message_t *message, uint32_t nonce);

int vip_get_type_header_service_id(vip_message_t *message, uint32_t **service_id, uint16_t service_num_len);
int vip_set_type_header_service_id(vip_message_t *message, uint32_t *service_id, uint16_t service_num_len);

int vip_get_type_header_vr_seq_num(vip_message_t *message, uint32_t *vr_seq_number);
int vip_set_type_header_vr_seq_num(vip_message_t *message, uint32_t vr_seq_number);

int vip_get_type_header_vg_seq_num(vip_message_t *message, uint32_t *vg_seq_number);
int vip_set_type_header_vg_seq_num(vip_message_t *message, uint32_t vg_seq_number);

int vip_set_payload(vip_message_t *message, void *payload, size_t payload_len);

int vip_set_service_list(vip_message_t *vip_pkt, char **service_list, size_t service_num);