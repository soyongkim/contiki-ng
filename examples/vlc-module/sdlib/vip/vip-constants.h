#ifndef VIP_CONSTANT
#define VIP_CONSTANT

#define VIP_MAX_SERVICE 65536
#define VIP_COMMON_HEADER_LEN 12

#define VIP_VR_ID_LEN 4
#define VIP_NO_ALLOCATED_VR_ID 0

#define VIP_MAX_UPLINK_ID 1024
#define VIP_SEQ_LEN 4
#define VIP_MAX_QUERY_SIZE 100
#define VIP_MAX_SEND_BUF_SIZE 16
#define VIP_MAX_PKT_SIZE 512

/* for simulation */
#define VIP_WINDOW_SIZE 8
#define VIP_SIMUL_DATA 50

/* for test, vg-id is fixed by 1 */
#define VIP_VG_ID 1

#define VIP_BROADCAST 0

#define VIP_ERROR 0
#define VIP_NO_ERROR 1

/* vip url */
#define VIP_VG_URL "vip/vg"
#define VIP_AA_URL "vip/aa"
#define VIP_VT_URL "vip/vt"
#define VIP_VR_URL "vip/vr"


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
  VIP_TYPE_VSD,                /* VLC Service Data */
  VIP_TYPE_VDA,               /* VLC service Data ACK */
  VIP_TYPE_ALLOC
} vip_message_type_t;

#endif