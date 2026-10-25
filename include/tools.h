#ifndef TOOLS_H
#define TOOLS_H

#include "led_strip.h"

#define LED_GPIO_PIN 8

extern led_strip_handle_t led;

void led_init(void);

void led_blink_task(void* pvParameters);

void program_state_post(program_state_t state);

void program_state_callback(TimerHandle_t xTimer);

esp_err_t tools_save_to_nvs(const char* namespace, const char* key, const void* data, size_t* size);

esp_err_t tools_read_from_nvs(const char* namespace, const char* key, void* data, size_t* size);

#endif // TOOLS_H