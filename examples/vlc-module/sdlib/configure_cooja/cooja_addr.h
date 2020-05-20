#include "net/ipv6/uip.h"

#define BROADCAST 0
#define BROADCAST_COAP_ADDR "coap://[ff02::1]" 
/**
 * 목적지를 원하는 노드로 바꾸기
 */
uip_ip6addr_t change_target(int node_id);

void make_coap_uri(char *src_coap_uri, int node_id);
void sky_mote_address(uip_ipaddr_t *dest_ipaddr, int node_id);
void cooja_mote_address(uip_ipaddr_t *dest_ipaddr, int node_id);