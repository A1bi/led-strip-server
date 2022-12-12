#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>

#include "utils.h"
#include "led.h"
#include "server.h"

static uint8_t running = 1;

static void exit_handler(int signum) {
  running = 0;
}

static void setup_handlers(void) {
  struct sigaction sa = {
    .sa_handler = exit_handler,
  };

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

void *receive_udp() {
  server_listen();
}

void parse_arguments(int argc, char *argv[]) {
  int i, channel_count = argc - 1, led_count = 0, led_pin = 0;

  if (channel_count < 1) {
    panic("No channel configuration specified.");
  } else if (channel_count > MAX_CHANNELS) {
    panic("Too many channel configurations specified.");
  }

  for (i = 0; i < channel_count; ++i) {
    char *val = strtok(argv[i + 1], ":");
    if (val != NULL) led_pin = atoi(val);

    val = strtok(NULL, ":");
    if (val != NULL) led_count = atoi(val);

    if (led_pin < 1 || led_count < 1) {
      panic("Invalid channel configuration.");
    }

    led_set_channel(i, led_pin, led_count);
  }
}

int main(int argc, char *argv[]) {
  int i, j;
  pthread_t server_thread;

  parse_arguments(argc, argv);
  setup_handlers();

  led_init();
  server_init();

  pthread_create(&server_thread, NULL, receive_udp, (void *)&server_thread);

  while (running) {
    for (i = 0; i < led_count(0); i++) {
      for (j = 0; j < led_count(0); j++) {
        led_set_color(0, j, i == j ? 0x00200000 : 0);
      }

      led_render();
      usleep(1000000 / 30); // 30 fps
    }
  }

  led_clear();
  led_close();

  return 0;
}
