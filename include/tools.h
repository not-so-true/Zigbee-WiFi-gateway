#ifndef TOOLS_H
#define TOOLS_H

#include "led_strip.h"

#define LED_GPIO_PIN 8

extern led_strip_handle_t led;

void post_program_state(program_state_t state);
void program_state_callback(TimerHandle_t xTimer);
void led_blink_task(void* pvParameters);

#endif // TOOLS_H