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
#include "net/nbr-table.h"
#include <stdio.h>
#include "project-conf.h"

/************************************************
 *                  Constants                   *
 ************************************************/
#define LOG_MODULE "Coordinator"
#define LOG_LEVEL LOG_LEVEL_DBG
#define UDP_PORT 1234
#define MAX_NODES 16
#define CHECK_INTERVAL (CLOCK_SECOND * 5)


/************************************************
 *                  Structs                     *
 ************************************************/
typedef struct {
  uint16_t tx_count;
  uint16_t rx_count;
  uip_ipaddr_t node_addr;
  uip_ipaddr_t parent_addr;
} node_stats_t;

/************************************************
 *              Global variables                *
 ************************************************/
static node_stats_t node_stats[MAX_NODES];
static struct simple_udp_connection udp_conn;


/************************************************
 *                  Functions                   *
 ************************************************/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen);

static void print_routing_table();

/************************************************
 *                  Processes                   *
 ************************************************/
PROCESS(coordinator_process, "Coordinator Process");
AUTOSTART_PROCESSES(&coordinator_process);

PROCESS_THREAD(coordinator_process, ev, data) {
  static struct etimer timer;
  PROCESS_BEGIN();

  LOG_INFO("Starting coordinator node...\r\n");
  NETSTACK_ROUTING.root_start();
  sixtop_add_sf(&sf_simple_driver);
  NETSTACK_MAC.on();

  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  etimer_set(&timer, CHECK_INTERVAL);
  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&timer)) {
      print_routing_table();
      etimer_reset(&timer);
    }
  }

  PROCESS_END();
}


/* UDP receive callback function */
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
  uint8_t node_id = data[0];
  
  if(node_id == 4 || node_id == 15 || node_id == 88 || node_id == 171 ) {
    uint16_t tx_count = (data[1] << 8) | data[2];
    int16_t real_temp = (data[11] << 8) | data[12];
    int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

    // Update transmission and reception counts
    node_stats[node_id].tx_count = tx_count;
    node_stats[node_id].rx_count++;

    // Set the node's own address and its parent address
    uip_ipaddr_copy(&node_stats[node_id].node_addr, sender_addr);
    memcpy(&node_stats[node_id].parent_addr, &data[13], sizeof(uip_ipaddr_t));

    // Calculate Packet Reception Ratio (PRR)
    int prr = (node_stats[node_id].tx_count > 0) ? 
              (int)((float)node_stats[node_id].rx_count / node_stats[node_id].tx_count * 100) : 0;

    LOG_INFO("Node %u | TX: %u | RX: %u | PRR: %d%% | RSSI: %d | Temperature: %dC\r\n",
             node_id, node_stats[node_id].tx_count, node_stats[node_id].rx_count,
             prr, rssi, real_temp);
  } else {
    LOG_ERR("Invalid node ID %u\r\n", node_id);
  }
}

/* Function to print routing table */
static void print_routing_table() {
  uip_ipaddr_t coordinator_addr;
  if(NETSTACK_ROUTING.get_root_ipaddr(&coordinator_addr)) {
    printf("Coordinator: ");
    uip_debug_ipaddr_print(&coordinator_addr);
    printf("\r\nRouting Table:\r\n");
  } else {
    printf("Coordinator address unavailable.\r\n");
  }

// Check and print information only for nodes 4, 15, 88, and 171
int nodes_to_check[] = {4, 15, 88, 171};

for(int j = 0; j < sizeof(nodes_to_check) / sizeof(nodes_to_check[0]); j++) {
    int node_id = nodes_to_check[j];
    if(node_stats[node_id].rx_count > 0) {
        printf("Node ID %d [", node_id);
        uip_debug_ipaddr_print(&node_stats[node_id].node_addr); // Print the node's own address
        printf("] via ");

        if(uip_is_addr_unspecified(&node_stats[node_id].parent_addr)) {
            printf("root"); // If no specific parent, assume root
        } else {
            uip_debug_ipaddr_print(&node_stats[node_id].parent_addr); // Print parent address if available
        }
        printf("\r\n");
    }
}
}