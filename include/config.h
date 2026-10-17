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
#include "nvs_flash.h"
#include "esp_coexist.h"

#define SYSTEM_BIT BIT0
#define ZIGBEE_BIT BIT1
#define WIFI_BIT BIT2
#define OTHER_BIT BIT3

static const char* TAG_SYSTEM = "---> SYSTEM";
static const char* TAG_LED = "---> LED";
static const char* TAG_WIFI = "---> WIFI";
static const char* TAG_ZIGBEE = "---> ZIGBEE";

typedef enum {IDLE, DONE, PROCESSING, ERROR} program_state_t;

extern EventGroupHandle_t event_group;
extern QueueHandle_t program_state_queue;
extern TimerHandle_t program_state_timer;

#endif // CONFIG_H