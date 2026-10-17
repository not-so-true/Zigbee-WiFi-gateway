#include "config.h"
#include "tools.h"
#include "wifi.h"
#include "zigbee.h"

EventGroupHandle_t event_group;
QueueHandle_t program_state_queue;
TimerHandle_t program_state_timer;

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_erase_partition("zb_storage")); // tylko dla testów!

    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_coex_wifi_i154_enable());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    event_group = xEventGroupCreate();

    led_init();
    xTaskCreate(led_blink_task, "led_blink", 2048, led, 2, NULL);

    xTaskCreate(zigbee_stack_task, "zigbee_stack", 8192, NULL, 8, NULL);

    // odczekanie na formację sieci Zigbee, ze względu na wymagające skanowanie kanałów
    xEventGroupWaitBits(event_group, ZIGBEE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    wifi_init();

    esp_netif_ip_info_t ipv4;
    esp_ip6_addr_t ipv6[CONFIG_LWIP_IPV6_NUM_ADDRESSES];

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        ESP_LOGI(TAG_SYSTEM, "Alive...");
    }
}