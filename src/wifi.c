#include "config.h"
#include "wifi.h"
#include "tools.h"

esp_netif_t* wifi_netif;
SemaphoreHandle_t netif_semaphore;

void wifi_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == IP_EVENT || event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_CONNECTED:
                post_program_state(DONE);
                if (xSemaphoreTake(netif_semaphore, portMAX_DELAY) == pdTRUE) {
                    esp_netif_create_ip6_linklocal(wifi_netif);
                    xSemaphoreGive(netif_semaphore);
                }

                ESP_LOGI(TAG_WIFI, "Connected successfully");
                break;
            case IP_EVENT_TX_RX:
                post_program_state(PROCESSING);
                ESP_LOGI(TAG_WIFI, "IP packet received/transmitted");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                post_program_state(ERROR);
                ESP_LOGI(TAG_WIFI, "Connection lost");
                break;
            default:
                ESP_LOGI(TAG_WIFI, "Unknown event: %d", event_id);
                break;
        }
    }
}
