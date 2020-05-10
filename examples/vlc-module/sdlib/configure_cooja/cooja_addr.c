#include "cooja_addr.h"
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
make_coap_uri(char *src_coap_uri, int node_id) {
  char coap_uri[25] = "coap://[fe:80::";
  char first[4], second[4], third[4], forth[4];
  
  sprintf(first, "%d", 200 + node_id);
  strcat(coap_uri, first);
  strcat(coap_uri, ":");
  sprintf(second, "%d", node_id);
  strcat(coap_uri, second);
  strcat(coap_uri, ":");
  sprintf(third, "%d", node_id);
  strcat(coap_uri, third);
  strcat(coap_uri, ":");
  sprintf(forth, "%d", node_id);
  strcat(coap_uri, forth);

  printf("RES:%s\n", coap_uri);
  strcpy(src_coap_uri, coap_uri);
}