#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event.h"

static const char* TAG_LED = "led";
static const char* TAG_WIFI = "wifi";
static const char* TAG_ZIGBEE = "zigbee";

typedef enum {IDLE, DONE, PROCESSING, ERROR} program_state_t;

extern QueueHandle_t program_state_queue;
extern TimerHandle_t program_state_timer;

#endif // CONFIG_H