#include "contiki.h"
#include "net/ipv6/uip.h"

#define BROADCAST 0

/**
 * 목적지를 원하는 노드로 바꾸기
 */
uip_ip6addr_t change_target(int node_id);



int make_basis_header(int* header, int type, int payload_length, int aa_id, int vt_id);

int make_type_specific_header(int type);