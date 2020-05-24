#include "cooja_addr.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"


uip_ipaddr_t
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
      /* for sky mote */
      //sky_mote_addree(&dest_ipaddr, node_id);

      /* for cooja mote */
      cooja_mote_address(&dest_ipaddr, node_id);

    }

    return dest_ipaddr;
}

void
sky_mote_address(uip_ipaddr_t *dest_ipaddr, int node_id) {
      dest_ipaddr->u16[4] = UIP_HTONS(0x0212);
      dest_ipaddr->u16[5] = UIP_HTONS(0x7400 + node_id);
      dest_ipaddr->u16[6] = UIP_HTONS(0x0000 + node_id);
      dest_ipaddr->u16[7] = UIP_HTONS(0x0000 + (256)*node_id + node_id);
}

void
cooja_mote_address(uip_ipaddr_t *dest_ipaddr, int node_id) {
      dest_ipaddr->u16[4] = UIP_HTONS(0x0200 + node_id);
      dest_ipaddr->u16[5] = UIP_HTONS(0x0000 + node_id);
      dest_ipaddr->u16[6] = UIP_HTONS(0x0000 + node_id);
      dest_ipaddr->u16[7] = UIP_HTONS(0x0000 + node_id);
}


void
make_coap_uri(char *src_coap_addr, int node_id) {
  if(!node_id) {
      strcpy(src_coap_addr, BROADCAST_COAP_ADDR);
  }
  else {
    char coap_uri[30] = "coap://[fe80::";
    char first[5], second[5], third[5], forth[5];

    sprintf(first, "%x", 512 + node_id);
    strcat(coap_uri, first);
    strcat(coap_uri, ":");
    sprintf(second, "%x", node_id);
    strcat(coap_uri, second);
    strcat(coap_uri, ":");
    sprintf(third, "%x", node_id);
    strcat(coap_uri, third);
    strcat(coap_uri, ":");
    sprintf(forth, "%x", node_id);
    strcat(coap_uri, forth);
    strcat(coap_uri, "]");

    strcpy(src_coap_addr, coap_uri);    
  }
}