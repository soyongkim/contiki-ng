#include "contiki.h"
#include "net/ipv6/uip.h"
#include "vip-constants.h"

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
    uint16_t service_num;
    uint32_t service_id[VIP_MAX_SERVICE];
    uint32_t vr_seq_number;
    uint32_t vg_seq_number;

    uint32_t payload_len;
    uint8_t *payload;
} vip_message_t;


void vip_init_message(vip_message_t *message, uint8_t type, uint16_t aa_id, uint16_t vt_id);

int vip_serialize_message(vip_message_t *message, uint8_t *buffer);
int vip_parse_common_header(vip_message_t *message, uint8_t *data, uint16_t data_len);

/* Type Specific Header */
uint8_t* vip_serialize_beacon(vip_message_t *message, uint8_t *buffer);
void vip_parse_beacon(vip_message_t *message);

uint8_t* vip_serialize_VRR(vip_message_t *message, uint8_t *buffer);
int vip_parse_VRR(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_VRA(vip_message_t *message, uint8_t *buffer);
int vip_parse_VRA(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_VRC(vip_message_t *message, uint8_t *buffer);
int vip_parse_VRC(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_REL(vip_message_t *message, uint8_t *buffer);
int vip_parse_REL(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_SER(vip_message_t *message, uint8_t *buffer);
int vip_parse_SER(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_SEA(vip_message_t *message, uint8_t *buffer);
int vip_parse_SEA(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_SEC(vip_message_t *message, uint8_t *buffer);
int vip_parse_SEC(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_SD(vip_message_t *message, uint8_t *buffer);
int vip_parse_SD(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_SDA(vip_message_t *message, uint8_t *buffer);
int vip_parse_SDA(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_VU(vip_message_t *message, uint8_t *buffer);
int vip_parse_VU(vip_message_t *message, uint8_t *buffer);

uint8_t* vip_serialize_VM(vip_message_t *message, uint8_t *buffer);
int vip_parse_VM(vip_message_t *message, uint8_t *buffer);


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

int vip_get_type_header_service_id(vip_message_t *message, uint32_t **service_id, uint16_t service_num_len);
int vip_set_type_header_service_id(vip_message_t *message, uint32_t *service_id, uint16_t service_num_len);

int vip_get_type_header_vr_seq_num(vip_message_t *message, uint32_t *vr_seq_number);
int vip_set_type_header_vr_seq_num(vip_message_t *message, uint32_t vr_seq_number);

int vip_get_type_header_vg_seq_num(vip_message_t *message, uint32_t *vg_seq_number);
int vip_set_type_header_vg_seq_num(vip_message_t *message, uint32_t vg_seq_number);

int vip_get_payload(vip_message_t *message, const uint8_t **payload);
int vip_set_payload(vip_message_t *message, const void *payload, size_t payload_len);

void vip_coap_response(vip_message_t *vip_pkt, char *taget_url);