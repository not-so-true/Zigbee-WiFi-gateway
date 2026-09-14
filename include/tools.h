#ifndef TOOLS_H
#define TOOLS_H

#include "led_strip.h"

extern led_strip_handle_t led;

void change_led_colour(led_strip_handle_t led, 
                        uint8_t r, 
                        uint8_t g, 
                        uint8_t b, 
                        uint32_t delay);

void blink_led_task(void* pvParameters);

#endif // TOOLS_H