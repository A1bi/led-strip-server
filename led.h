#include <ws2811.h>

#define DMA            10
#define TARGET_FREQ    WS2811_TARGET_FREQ
#define STRIP_TYPE     WS2811_STRIP_GRB
#define MAX_CHANNELS   2

typedef ws2811_led_t led_color_t;

void led_init();
void led_set_channel(int channel, int pin, int led_count);
void led_set_color(int channel, int pixel, led_color_t color);
void led_source_tick();
void led_render();
void led_render_loop();
int led_count(int channel);
void led_clear();
void led_close();
