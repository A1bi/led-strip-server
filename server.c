#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

#include "server.h"
#include "utils.h"
#include "led.h"

#define MAX_PACKET_SIZE 1452 // 1500 (Ethernet) - 40 (IPv6 header) - 8 (UDP header)

int sockfd;

void server_init() {
  struct sockaddr_in6 servaddr;

  if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
    panic("Socket creation failed.");
    panic("");
  }

  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin6_family = AF_INET6;
  servaddr.sin6_addr = in6addr_any;
  servaddr.sin6_port = htons(PORT);

  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Could not bind to port");
    panic("");
  }
}

void server_process_packet(char *data, uint16_t bytes) {
  uint16_t i = 0;

  for (int i = 0; (i+2) < bytes; i += 3) {
    led_color_t color = (uint32_t)data[i] << 16 | (uint32_t)data[i+1] << 8 | (uint32_t)data[i+2];
    led_set_color(0, i / 3, color);
  }

  led_render();
}

void server_listen() {
  uint16_t bytes;
  char buffer[MAX_PACKET_SIZE];
  struct sockaddr_in6 cliaddr;
  int len = sizeof(cliaddr);

  memset(&cliaddr, 0, len);

  while (1) {
    bytes = recvfrom(sockfd, (char *)buffer, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
    server_process_packet(buffer, bytes);
  }
}
