#include "contiki.h"
#include "net/routing/routing.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "sf-simple.h"
#include "sys/log.h"
#include "net/ipv6/simple-udp.h"

#define LOG_MODULE "Coordinator"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

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
  LOG_INFO("Received data from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_(": %.*s\r\n", datalen, (char *) data);
}


PROCESS_THREAD(coordinator_process, ev, data) {
  PROCESS_BEGIN();

  LOG_INFO("Starting coordinator node...\r\n");
  
  /* Thiết lập node này làm root */
  NETSTACK_ROUTING.root_start();
  sixtop_add_sf(&sf_simple_driver);
  NETSTACK_MAC.on();

  /* Thiết lập UDP */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  while(1) {
    PROCESS_YIELD();
    
    // Sẵn sàng nhận dữ liệu từ các sensor node
    LOG_INFO("Coordinator ready to receive sensor data...\r\n");
  }

  PROCESS_END();
}
