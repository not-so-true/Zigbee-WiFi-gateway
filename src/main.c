#include "config.h"
#include "tools.h"
#include "wifi.h"

led_strip_handle_t led;
esp_netif_t* wifi_netif;
SemaphoreHandle_t netif_semaphore;
QueueHandle_t program_state_queue;

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

void app_main(void) {   
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    netif_semaphore = xSemaphoreCreateMutex();

    led_init();
    wifi_init();

    program_state_t program_state = PROCESSING;
    program_state_queue = xQueueCreate(3, sizeof(program_state_t));
    xQueueSend(program_state_queue, &program_state, 0);
    xTaskCreate(blink_led_task, "blink_led", 1024, led, 2, NULL);

    esp_wifi_connect();

    esp_netif_ip_info_t ipv4;
    esp_ip6_addr_t ipv6[CONFIG_LWIP_IPV6_NUM_ADDRESSES];
    vTaskDelay(pdMS_TO_TICKS(20000));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (xSemaphoreTake(netif_semaphore, portMAX_DELAY) == pdTRUE) {
            esp_netif_get_ip_info(wifi_netif, &ipv4);
            esp_netif_get_all_ip6(wifi_netif, ipv6);
            xSemaphoreGive(netif_semaphore);
        }

        ESP_LOGI(TAG_WIFI, "Adres IP: " IPSTR, IP2STR(&ipv4.ip));
        ESP_LOGI(TAG_WIFI, "Maska:    " IPSTR, IP2STR(&ipv4.netmask));
        ESP_LOGI(TAG_WIFI, "Brama:    " IPSTR, IP2STR(&ipv4.gw));
        ESP_LOGI(TAG_WIFI, "IPv6 link-local: " IPV6STR, IPV62STR(ipv6[0]));
        ESP_LOGI(TAG_WIFI, "IPv6 jakieś: " IPV6STR, IPV62STR(ipv6[1]));
    }
}