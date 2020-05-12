#define VIP_MAX_SERVICE 65536
#define VIP_COMMON_HEADER_LEN 8

#define VIP_VR_ID_LEN 4
#define VIP_NO_ALLOCATED_VR_ID 0

#define VIP_MAX_UPLINK_ID 1024
#define VIP_SEQ_LEN 4

#define VIP_SERVICE_NUM 2
#define VIP_SERVICE_ID_LEN 4
#define VIP_PAYLOAD_LEN_POSITION 8


#define VIP_ERROR 0
#define VIP_NO_ERROR 1

#define VIP_BROADCAST_URI "coap://[ff01::2]"

/* VIP message types */
typedef enum {
  VIP_TYPE_BEACON = 0,             /* Beacon */
  VIP_TYPE_VRR,                /* VR Registration Request */
  VIP_TYPE_VRA,                /* VR Registration ACK */
  VIP_TYPE_VRC,                /* VR Registration Confirm */
  VIP_TYPE_REL,                /* VR Release */
  VIP_TYPE_SER,                /* Session Establishment Request */
  VIP_TYPE_SEA,                /* Session Establishment ACK */
  VIP_TYPE_SEC,                /* Session Establishment Confirm */
  VIP_TYPE_SD,                 /* Service Data */
  VIP_TYPE_SDA,                /* Service Data ACK */
  VIP_TYPE_VU,                 /* VR Update */
  VIP_TYPE_VM                  /* VR Migration */
} vip_message_type_t;
