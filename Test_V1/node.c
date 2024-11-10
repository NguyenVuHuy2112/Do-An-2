#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "sf-simple.h"
#include "sys/log.h"
#include "sys/node-id.h"
#include "net/ipv6/simple-udp.h"
#include "sys/rtimer.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-debug.h"
#include <stdlib.h>
#include <stdio.h>
#include "project-conf.h"

#define LOG_MODULE "Sensor Node"
#define LOG_LEVEL LOG_LEVEL_DBG
#define UDP_PORT 1234
#define CHECK_INTERVAL (CLOCK_SECOND * 5)

static struct simple_udp_connection udp_conn;
static uint16_t node_tx_count = 0;

PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

static void send_temperature_data() {
  uip_ipaddr_t dest_ipaddr;
  int8_t real_temp = 20 + (rand() % 100) / 10;
  LOG_INFO("Node %u: Temperature = %d C\r\n", node_id, real_temp);

  node_tx_count++;
  uint64_t send_time = tsch_get_network_uptime_ticks();
  
  uint8_t payload[13];
  payload[0] = node_id;
  payload[1] = (node_tx_count >> 8) & 0xFF;
  payload[2] = node_tx_count & 0xFF;
  memcpy(&payload[3], &send_time, sizeof(send_time));

  payload[11] = (real_temp >> 8) & 0xFF;
  payload[12] = real_temp & 0xFF;

  if(NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
    simple_udp_sendto(&udp_conn, payload, sizeof(payload), &dest_ipaddr);
    LOG_INFO("Node %u: Sent data with TX count = %u | Send Time: %lu | Temperature: %d°C\r\n", 
             node_id, node_tx_count, (unsigned long)send_time, real_temp);
  } else {
    LOG_ERR("Failed to get coordinator IP address.\r\n");
  }
}

PROCESS_THREAD(sensor_node_process, ev, data) {
  static struct etimer et;
  static struct etimer timer;

  PROCESS_BEGIN();

  sixtop_add_sf(&sf_simple_driver);
  NETSTACK_MAC.on();
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  etimer_set(&timer, CHECK_INTERVAL);
  while(1) {
    etimer_set(&et, CLOCK_SECOND * 20);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      send_temperature_data();
    

    if(etimer_expired(&timer)) {
      rpl_dag_t *dag = rpl_get_any_dag();
      if(dag != NULL) {
          printf("Node rank: %u\n", dag->rank);
          if(dag->preferred_parent != NULL) {
              uip_ipaddr_t *parent_addr = rpl_neighbor_get_ipaddr(dag->preferred_parent);
              if(parent_addr != NULL) {
                  printf("Parent address: ");
                  uip_debug_ipaddr_print(parent_addr);
                  printf("\n");
              } else {
                  printf("Parent address is not available\n");
              }
          } else {
              printf("No preferred parent\n");
          }
      } else {
          printf("No DAG available\n");
      }
      etimer_reset(&timer);
    }
  }

  PROCESS_END();
}
