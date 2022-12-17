#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "server.h"
#include "utils.h"
#include "led.h"

#define SERVER_CONTROL_PACKET_SIZE 6
#define SERVER_MAX_LED_PACKET_SIZE 1452 // 1500 (Ethernet) - 40 (IPv6 header) - 8 (UDP header)

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
  struct sockaddr_in6 interface = {0};
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
  if (bytes < 6) return; // 1 (channel) + 2 (offset) + 3 (first color)

  uint8_t channel = data[0];
  uint16_t offset = ntohs(*(uint16_t*)&(data[1]));
  char *color_data = &(data[3]);
  uint16_t color_bytes = bytes - 2;

  for (uint16_t i = 0; (i+2) < color_bytes; i += 3) {
    uint16_t led_index = offset + i / 3;
    if (led_index > led_count(channel)) break;

    led_color_t color = (led_color_t)color_data[i] << 16 | (led_color_t)color_data[i+1] << 8 | (led_color_t)color_data[i+2];
    led_set_color(channel, led_index, color);
  }
}

void server_recv_control() {
  char buffer_req[SERVER_CONTROL_PACKET_SIZE], buffer_res[4];
  size_t size_res;
  int conn_fd;
  struct sockaddr_in6 client_addr = {0};
  socklen_t client_size = sizeof(client_addr);

  while (true) {
    if ((conn_fd = accept(control_fd, (struct sockaddr *)&client_addr, &client_size)) < 0) {
      printf("Failed to accept connection.");
      continue;
    }

    size_res = 4;
    strncpy(buffer_res, "NOK", size_res);

    if (read(conn_fd, buffer_req, sizeof(buffer_req)) == SERVER_CONTROL_PACKET_SIZE) {
      led_close();
      for (uint8_t i = 0; i < LED_MAX_CHANNELS; i++) {
        if (buffer_req[i*2] > 0) {
          led_set_channel(i, buffer_req[i*2], buffer_req[i*2+1]);
        }
      }
      led_init();

      led_set_fps((int)buffer_req[4]);
      led_toggle_activity_indicator((bool)buffer_req[5]);

      printf("Received control packet:\nchan 1 = %d:%d\nchan 2 = %d:%d\nfps = %d\nactivity = %d\n",
             buffer_req[0], buffer_req[1], buffer_req[2], buffer_req[3], buffer_req[4], buffer_req[5]);
      size_res = 3;
      strncpy(buffer_res, "OK", size_res);
    }

    write(conn_fd, buffer_res, size_res);
    close(conn_fd);
  }
}

void server_recv_led() {
  char buffer[SERVER_MAX_LED_PACKET_SIZE];
  struct sockaddr_in6 client_addr = {0};
  socklen_t client_size = sizeof(client_addr);

  while (true) {
    uint16_t bytes = recvfrom(led_fd, (char *)buffer, SERVER_MAX_LED_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &client_size);
    server_process_packet(buffer, bytes);
    led_source_tick();
  }
}
