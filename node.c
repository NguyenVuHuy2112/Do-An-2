#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "sf-simple.h"
#include "sys/log.h"
#include "sys/node-id.h"
#include "net/ipv6/simple-udp.h"
#include <stdlib.h>

#define LOG_MODULE "Sensor Node"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

static struct simple_udp_connection udp_conn;

PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

/* Hàm gửi dữ liệu nhiệt độ tới coordinator qua UDP */
static void send_temperature_data() {
  uip_ipaddr_t dest_ipaddr;

  /* Giả lập nhiệt độ trong khoảng 20 - 30 độ C */
  int8_t real_temp = 20.0 + (rand() % 100) / 10.0;
  LOG_INFO("Node %u: Temperature = %d C\r\n", node_id, real_temp);

  /* Chuyển đổi nhiệt độ thành chuỗi */
  char temp_str[32];
  snprintf(temp_str, sizeof(temp_str), "Temp: %d C", real_temp);

  /* Gửi dữ liệu tới địa chỉ của coordinator */
  if(NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
    simple_udp_sendto(&udp_conn, temp_str, strlen(temp_str), &dest_ipaddr);
  } else {
    LOG_ERR("Failed to get coordinator IP address.\r\n");
  }
}

PROCESS_THREAD(sensor_node_process, ev, data) {
  static struct etimer et;

  PROCESS_BEGIN();

  /* Thiết lập 6top */
  sixtop_add_sf(&sf_simple_driver);
  NETSTACK_MAC.on();

  /* Thiết lập UDP */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 10); // Đọc nhiệt độ mỗi 10 giây
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    send_temperature_data();
  }

  PROCESS_END();
}
