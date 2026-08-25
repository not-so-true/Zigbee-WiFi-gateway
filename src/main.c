#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event.h"

#include "led_strip.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define LED_GPIO_PIN 8
#define WIFI_SSID "Kamil's Galaxy S23+"
#define WIFI_PASS "1234abcd"

typedef enum {IDLE, PROCESSING, ERROR} program_state_t;

const char* TAG_LED = "led";
const char* TAG_WIFI = "wifi";

QueueHandle_t program_state_queue;

led_strip_handle_t led;
esp_netif_t* wifi_netif;

void netif_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    program_state_t program_state = IDLE;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG_WIFI, "Connecting...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG_WIFI, "Disconnected");
                break;
            default:
                ESP_LOGI(TAG_WIFI, "WIFI_EVENT: %d", event_id);
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                program_state = IDLE;
                xQueueSend(program_state_queue, &program_state, 0);
                ESP_LOGI(TAG_WIFI, "Connected successfully");
                break;
            default:
                ESP_LOGI(TAG_WIFI, "IP_EVENT: %d", event_id);
                break;
        }
    }
}

void led_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO_PIN,
        .max_leds = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 0 //default value
    };

    esp_err_t check = led_strip_new_rmt_device(&strip_config, &rmt_config, &led);
    if (check != ESP_OK)
        ESP_LOGE(TAG_LED, "Failed to initialize");
    else
        ESP_LOGI(TAG_LED, "Initialized successfully");
}

void wifi_init(void) {
    wifi_netif = esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .failure_retry_cnt = 8
        },
    };
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &netif_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &netif_handler, NULL, NULL);
    
    ESP_LOGI(TAG_WIFI, "Initialized successfully");
}

void change_led_colour(led_strip_handle_t led, uint8_t r, uint8_t g, uint8_t b, uint32_t delay) {
    if (delay == 0) {
        led_strip_set_pixel(led, 0, r, g, b);
        led_strip_refresh(led);
    }
    else {
        led_strip_set_pixel(led, 0, r, g, b);
        led_strip_refresh(led);
        vTaskDelay(pdMS_TO_TICKS(delay));

        led_strip_clear(led);
        led_strip_refresh(led);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void blink_led_task(void* pvParameters) {
    led_strip_handle_t led = (led_strip_handle_t) pvParameters;
    program_state_t program_state;

    while (1) {
        xQueueReceive(program_state_queue, &program_state, 0);
        
        switch (program_state) {
        case IDLE:
            change_led_colour(led, 0, 20, 0, 0);
            break;
        case PROCESSING:
            change_led_colour(led, 0, 0, 20, 500);
            break;
        case ERROR:
            change_led_colour(led, 20, 0, 0, 100);
            break;
        }
    }
}

void app_main(void) {   
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    led_init();
    wifi_init();

    program_state_t program_state = PROCESSING;
    program_state_queue = xQueueCreate(3, sizeof(program_state_t));
    xQueueSend(program_state_queue, &program_state, 0);
    xTaskCreate(blink_led_task, "blink_led", 1024, led, 2, NULL);

    esp_wifi_connect();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}