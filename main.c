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

pthread_t led_render_thread, server_thread_led, server_thread_control;

static void exit_handler(int signum) {
  pthread_cancel(server_thread_control);
  pthread_cancel(server_thread_led);
  pthread_cancel(led_render_thread);
}

static void setup_handlers(void) {
  struct sigaction sa = {
    .sa_handler = exit_handler,
  };

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

void *render_loop() {
  led_render_loop();
}

void *recv_control() {
  server_recv_control();
}

void *recv_led() {
  server_recv_led();
}

void parse_arguments(int argc, char *argv[]) {
  uint8_t channel_count = argc - 1;
  if (channel_count < 1) {
    led_set_channel(0, 18, 24);
    led_set_channel(1, 19, 0);
    printf("No channel configuration specified. Using default: 18:24, 19:0.\n");

  } else if (channel_count > LED_MAX_CHANNELS) {
    panic("Too many channel configurations specified.");

  } else {
    for (uint8_t i = 0; i < channel_count; ++i) {
      uint8_t led_pin = 0;
      uint16_t led_count = 0;
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
}

int main(int argc, char *argv[]) {
  parse_arguments(argc, argv);
  setup_handlers();

  led_init();
  server_init();

  pthread_create(&led_render_thread, NULL, render_loop, (void *)&led_render_thread);
  pthread_create(&server_thread_control, NULL, recv_control, (void *)&server_thread_control);
  pthread_create(&server_thread_led, NULL, recv_led, (void *)&server_thread_led);
  pthread_join(led_render_thread, (void *)&led_render_thread);
  pthread_join(server_thread_control, (void *)&server_thread_control);
  pthread_join(server_thread_led, (void *)&server_thread_led);

  server_close();
  led_clear();
  led_close();

  return 0;
}
