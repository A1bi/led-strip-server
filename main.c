#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <ws2811.h>

#define GPIO_PIN       18
#define LED_COUNT      24
#define DMA            10
#define TARGET_FREQ    WS2811_TARGET_FREQ
#define STRIP_TYPE     WS2811_STRIP_GRB

ws2811_t ledstring = {
  .freq = TARGET_FREQ,
  .dmanum = DMA,
  .channel = {
    [0] = {
      .gpionum = GPIO_PIN,
      .invert = 0,
      .count = LED_COUNT,
      .strip_type = STRIP_TYPE,
      .brightness = 255,
    },
  },
};

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

int main(int argc, char *argv[]) {
  int i, j;
  ws2811_return_t ret;

  setup_handlers();

  if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
    return ret;
  }

  while (running) {
    for (i = 0; i < LED_COUNT; i++) {
      for (j = 0; j < LED_COUNT; j++) {
        ledstring.channel[0].leds[j] = i == j ? 0x00200000 : 0;
      }

      if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
        break;
      }

      // 30 fps
      usleep(1000000 / 30);
    }
  }

  // clear LEDs
  for (i = 0; i < LED_COUNT; i++) {
    ledstring.channel[0].leds[i] = 0;
  }
  ws2811_render(&ledstring);

  ws2811_fini(&ledstring);

  return ret;
}
