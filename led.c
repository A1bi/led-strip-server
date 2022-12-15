#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "led.h"

#define LED_ACTIVITY_INTERVAL 50
#define LED_ACTIVITY_COLOR 0x00800000
#define LED_SOURCE_ACTIVITY_TIMEOUT 100

#define TIMEDIFF_MS(tv1, tv2) ((tv1.tv_sec * 1000 + tv1.tv_usec / 1000) - (tv2.tv_sec * 1000 + tv2.tv_usec / 1000))

ws2811_t leds = {
  .freq = TARGET_FREQ,
  .dmanum = DMA
};

bool activity_indicator_enabled = false;
int fps = 24;
struct timeval last_source_activity = {0}, last_general_activity = {0};
bool last_general_activity_state = false, last_source_activity_state = false;

void led_init() {
  ws2811_return_t result = ws2811_init(&leds);
  if (result != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(result));
  }
}

void led_set_channel(uint8_t channel, uint8_t pin, uint16_t led_count) {
  leds.channel[channel].gpionum = pin;
  leds.channel[channel].count = led_count;
  leds.channel[channel].invert = 0;
  leds.channel[channel].strip_type = STRIP_TYPE;
  leds.channel[channel].brightness = 255;
}

void led_set_color(uint8_t channel, uint16_t pixel, led_color_t color) {
  leds.channel[channel].leds[pixel] = color;
}

void led_toggle_activity_color(uint16_t pixel, bool toggle) {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    led_set_color(i, pixel, toggle ? LED_ACTIVITY_COLOR : 0);
  }
}

void led_set_fps(int f) {
  if (f > 0) fps = f;
}

void led_toggle_activity_indicator(bool enabled) {
  activity_indicator_enabled = enabled;

  if (!enabled) {
    led_toggle_activity_color(0, false);
    led_toggle_activity_color(1, false);
  }
}

void led_source_tick() {
  gettimeofday(&last_source_activity, NULL);
}

void led_indicate_activity() {
  struct timeval now = {0};

  gettimeofday(&now, NULL);

  if (TIMEDIFF_MS(now, last_general_activity) > LED_ACTIVITY_INTERVAL) {
    // regular activity
    last_general_activity_state = !last_general_activity_state;
    gettimeofday(&last_general_activity, NULL);

    // source activity
    if (TIMEDIFF_MS(now, last_source_activity) > LED_SOURCE_ACTIVITY_TIMEOUT) {
      // timeout
      last_source_activity_state = false;
    } else {
      last_source_activity_state = last_general_activity_state;
    }

    led_toggle_activity_color(0, last_general_activity_state);
    led_toggle_activity_color(1, last_source_activity_state);
  }
}

void led_render() {
  ws2811_return_t result = ws2811_render(&leds);
  if (result != WS2811_SUCCESS) {
    fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(result));
  }
}

void led_render_loop() {
  while (true) {
    if (activity_indicator_enabled) {
      led_indicate_activity();
    }

    led_render();

    usleep(1000000 / fps);
  }
}

uint16_t led_count(uint8_t channel) {
  if (channel >= MAX_CHANNELS) return 0;

  return leds.channel[channel].count;
}

void led_clear() {
  for (uint16_t i = 0; i < led_count(0); i++) {
    leds.channel[0].leds[i] = 0;
  }
  led_render();
}

void led_close() {
  ws2811_fini(&leds);
}
