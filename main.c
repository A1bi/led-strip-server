#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <ws2811.h>

#include "utils.h"
#include "server.h"

#define DMA            10
#define TARGET_FREQ    WS2811_TARGET_FREQ
#define STRIP_TYPE     WS2811_STRIP_GRB
#define MAX_CHANNELS   2

static uint8_t running = 1;
static uint8_t channel_count = 0;

ws2811_t leds = {
  .freq = TARGET_FREQ,
  .dmanum = DMA
};

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

int main(int argc, char *argv[]) {
  int i, j, led_count = 0, led_pin = 0;
  pthread_t server_thread;
  ws2811_return_t ret;

  channel_count = argc - 1;

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

    leds.channel[i] = (ws2811_channel_t) {
      .gpionum = led_pin,
      .invert = 0,
      .count = led_count,
      .strip_type = STRIP_TYPE,
      .brightness = 255
    };
  }

  setup_handlers();

  if ((ret = ws2811_init(&leds)) != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
    return ret;
  }

  server_init();
  pthread_create(&server_thread, NULL, receive_udp, (void *)&server_thread);

  while (running) {
    for (i = 0; i < leds.channel[0].count; i++) {
      for (j = 0; j < leds.channel[0].count; j++) {
        leds.channel[0].leds[j] = i == j ? 0x00200000 : 0;
      }

      if ((ret = ws2811_render(&leds)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
        break;
      }

      // 30 fps
      usleep(1000000 / 30);
    }
  }

  // clear LEDs
  for (i = 0; i < leds.channel[0].count; i++) {
    leds.channel[0].leds[i] = 0;
  }
  ws2811_render(&leds);

  ws2811_fini(&leds);

  return ret;
}
