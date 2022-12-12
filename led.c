#include <stdio.h>

#include "led.h"

ws2811_t leds = {
  .freq = TARGET_FREQ,
  .dmanum = DMA
};

ws2811_return_t ret;

void led_init() {
  if ((ret = ws2811_init(&leds)) != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
  }
}

void led_set_channel(int channel, int pin, int led_count) {
  leds.channel[channel] = (ws2811_channel_t) {
    .gpionum = pin,
    .invert = 0,
    .count = led_count,
    .strip_type = STRIP_TYPE,
    .brightness = 255
  };
}

void led_set_color(int channel, int pixel, led_color_t color) {
  leds.channel[channel].leds[pixel] = color;
}

void led_render() {
  if ((ret = ws2811_render(&leds)) != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
  }
}

int led_count(int channel) {
  return leds.channel[channel].count;
}

void led_clear() {
  int i = 0;

  for (i = 0; i < led_count(0); i++) {
    leds.channel[0].leds[i] = 0;
  }
  led_render();
}

void led_close() {
  ws2811_fini(&leds);
}