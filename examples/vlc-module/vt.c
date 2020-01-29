#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");

}

static uip_ip6addr_t
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


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      /* 목적지 원하는 곳으로 바꾸기 */
      dest_ipaddr = change_target(0);
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
