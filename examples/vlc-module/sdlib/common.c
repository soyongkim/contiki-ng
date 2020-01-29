#include "common.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"

uip_ip6addr_t
change_target(int node_id) {
    const uip_ipaddr_t *default_prefix = uip_ds6_default_prefix();
    uip_ipaddr_t dest_ipaddr;
    uip_ip6addr_copy(&dest_ipaddr, default_prefix);

    if(node_id == 0) {
      dest_ipaddr.u16[0] = UIP_HTONS(0xFF02);
      dest_ipaddr.u16[1] = UIP_HTONS(0x0000);
      dest_ipaddr.u16[2] = UIP_HTONS(0x0000);
      dest_ipaddr.u16[3] = UIP_HTONS(0x0000); 
      dest_ipaddr.u16[4] = UIP_HTONS(0x0000);
      dest_ipaddr.u16[5] = UIP_HTONS(0x0000);
      dest_ipaddr.u16[6] = UIP_HTONS(0x0000);
      dest_ipaddr.u16[7] = UIP_HTONS(0x0001);
    }
    else {
      dest_ipaddr.u16[4] = UIP_HTONS(0x0212);
      dest_ipaddr.u16[5] = UIP_HTONS(0x7400 + node_id);
      dest_ipaddr.u16[6] = UIP_HTONS(0x0000 + node_id);
      dest_ipaddr.u16[7] = UIP_HTONS(0x0000 + (256)*node_id + node_id);
    }

    return dest_ipaddr;
}