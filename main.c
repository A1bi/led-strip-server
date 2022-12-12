#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <ws2811.h>

#define GPIO_PIN       18
#define DMA            10
#define TARGET_FREQ    WS2811_TARGET_FREQ
#define STRIP_TYPE     WS2811_STRIP_GRB

static uint8_t running = 1;
static uint8_t led_count = 0;

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

void setup_leds() {
  leds.channel[0] = (ws2811_channel_t) {
    .gpionum = GPIO_PIN,
    .invert = 0,
    .count = led_count,
    .strip_type = STRIP_TYPE,
    .brightness = 255
  };
}

int main(int argc, char *argv[]) {
  int i, j;
  ws2811_return_t ret;

  if (argc > 1) led_count = atoi(argv[1]);
  if (led_count < 1) {
    printf("Error: Number of LEDs not specified.\n");
    exit(1);
  }

  setup_handlers();
  setup_leds();

  if ((ret = ws2811_init(&leds)) != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
    return ret;
  }

  while (running) {
    for (i = 0; i < led_count; i++) {
      for (j = 0; j < led_count; j++) {
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
  for (i = 0; i < led_count; i++) {
    leds.channel[0].leds[i] = 0;
  }
  ws2811_render(&leds);

  ws2811_fini(&leds);

  return ret;
}
