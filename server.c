#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "server.h"
#include "utils.h"
#include "led.h"

#define CONTROL_PACKET_SIZE 6
#define MAX_LED_PACKET_SIZE 1452 // 1500 (Ethernet) - 40 (IPv6 header) - 8 (UDP header)

int control_fd, led_fd;

void server_init_control_interface(struct sockaddr_in6 *interface) {
  if ((control_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
    ppanic("TCP socket creation failed");
  }

  if ((bind(control_fd, (struct sockaddr *)interface, sizeof(*interface))) != 0) {
    ppanic("Could not bind to control interface port");
  }

  if ((listen(control_fd, 3)) != 0) {
    ppanic("Failed to failed on control interface port");
  }
}

void server_init_led_interface(struct sockaddr_in6 *interface) {
  if ((led_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    ppanic("UDP socket creation failed");
  }

  if (bind(led_fd, (struct sockaddr *)interface, sizeof(*interface)) < 0) {
    ppanic("Could not bind to LED interface port");
  }
}

void server_init() {
  struct sockaddr_in6 interface;

  memset(&interface, 0, sizeof(interface));

  interface.sin6_family = AF_INET6;
  interface.sin6_addr = in6addr_any;
  interface.sin6_port = htons(PORT);

  server_init_control_interface(&interface);
  server_init_led_interface(&interface);
}

void server_close() {
  close(control_fd);
  close(led_fd);
}

void server_process_packet(char *data, uint16_t bytes) {
  uint16_t i = 0;

  for (int i = 0; (i+2) < bytes; i += 3) {
    led_color_t color = (uint32_t)data[i] << 16 | (uint32_t)data[i+1] << 8 | (uint32_t)data[i+2];
    led_set_color(0, i / 3, color);
  }
}

void server_recv_control() {
  char buffer_req[CONTROL_PACKET_SIZE], buffer_res[3];
  struct sockaddr_in6 client_addr;
  int conn_fd, len = sizeof(client_addr), i;

  memset(&client_addr, 0, len);

  while (1) {
    if ((conn_fd = accept(control_fd, (struct sockaddr *)&client_addr, &len)) < 0) {
      printf("Failed to accept connection.");
      continue;
    }

    strncpy(buffer_res, "NOK", 3);

    if (read(conn_fd, buffer_req, sizeof(buffer_req)) == CONTROL_PACKET_SIZE) {
      led_close();
      for (i = 0; i < MAX_CHANNELS; i++) {
        if (buffer_req[i*2] > 0) {
          led_set_channel(i, buffer_req[i*2], buffer_req[i*2+1]);
        }
      }
      led_init();

      led_set_fps((int)buffer_req[4]);
      led_toggle_activity_indicator((bool)buffer_req[5]);

      printf("Received control packet:\nchan 1 = %d:%d\nchan 2 = %d:%d\nfps = %d\nactivity = %d\n",
             buffer_req[0], buffer_req[1], buffer_req[2], buffer_req[3], buffer_req[4], buffer_req[5]);
      strncpy(buffer_res, "OK\0", 3);
    }

    write(conn_fd, buffer_res, sizeof(buffer_res));
    close(conn_fd);
  }
}

void server_recv_led() {
  uint16_t bytes;
  char buffer[MAX_LED_PACKET_SIZE];
  struct sockaddr_in6 client_addr;
  int len = sizeof(client_addr);

  memset(&client_addr, 0, len);

  while (1) {
    bytes = recvfrom(led_fd, (char *)buffer, MAX_LED_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
    server_process_packet(buffer, bytes);
    led_source_tick();
  }
}
