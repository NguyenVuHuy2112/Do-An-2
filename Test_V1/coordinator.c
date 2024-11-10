#include "contiki.h"
#include "net/routing/routing.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "sf-simple.h"
#include "sys/log.h"
#include "net/ipv6/simple-udp.h"
#include "sys/rtimer.h"
#include "sys/node-id.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-debug.h"
#include <stdio.h>
#include "project-conf.h"

#define LOG_MODULE "Coordinator"
#define LOG_LEVEL LOG_LEVEL_DBG
#define UDP_PORT 1234
#define MAX_NODES 16
#define CHECK_INTERVAL (CLOCK_SECOND * 5)

typedef struct {
  uint16_t tx_count;
  uint16_t rx_count;
} node_stats_t;

static node_stats_t node_stats[MAX_NODES];
static struct simple_udp_connection udp_conn;

PROCESS(coordinator_process, "Coordinator Process");
AUTOSTART_PROCESSES(&coordinator_process);

/* Hàm callback nhận dữ liệu UDP */
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
  LOG_INFO("Received UDP packet from node\n");
  uint8_t node_id = data[0];
  uint16_t tx_count = (data[1] << 8) | data[2];
  int16_t real_temp = (data[11] << 8) | data[12];
  uint64_t arrival_time = tsch_get_network_uptime_ticks();
  int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  if(node_id < MAX_NODES) {
    node_stats[node_id].tx_count = tx_count;
    node_stats[node_id].rx_count++;

    int prr = (node_stats[node_id].tx_count > 0) ? 
              (int)((float)node_stats[node_id].rx_count / node_stats[node_id].tx_count * 100) : 0;

    if(datalen >= 13) {
      uint64_t send_time;
      memcpy(&send_time, &data[3], sizeof(send_time));
      uint64_t delay = arrival_time - send_time;

      LOG_INFO("Node %u | TX: %u | RX: %u | PRR: %d%% | RSSI: %d | Latency: %lu ticks | Temperature: %d°C\r\n",
               node_id, node_stats[node_id].tx_count, node_stats[node_id].rx_count,
               prr, rssi, (unsigned long)delay, real_temp);
    }
  } else {
    LOG_ERR("Invalid node ID %u\r\n", node_id);
  }
}

PROCESS_THREAD(coordinator_process, ev, data) {
  static struct etimer timer;
  PROCESS_BEGIN();

  LOG_INFO("Starting coordinator node...\r\n");
  printf("Node ID: %u\n", node_id);
  NETSTACK_ROUTING.root_start();
  sixtop_add_sf(&sf_simple_driver);
  NETSTACK_MAC.on();

  for(int i = 0; i < MAX_NODES; i++) {
    node_stats[i].tx_count = 0;
    node_stats[i].rx_count = 0;
  }

  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  etimer_set(&timer, CHECK_INTERVAL);
  while(1) {
    PROCESS_YIELD();

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
