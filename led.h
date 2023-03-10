#include <stdbool.h>
#include <ws2811.h>

#define LED_MAX_CHANNELS 2

typedef ws2811_led_t led_color_t;

void led_init();
void led_set_channel(uint8_t channel, uint8_t pin, uint16_t led_count);
void led_set_color(uint8_t channel, uint16_t pixel, led_color_t color);
void led_set_fps(uint8_t fps);
void led_toggle_activity_indicator(bool enabled);
void led_source_tick();
void led_render();
void led_render_loop();
uint16_t led_count(uint8_t channel);
void led_clear();
void led_close();
