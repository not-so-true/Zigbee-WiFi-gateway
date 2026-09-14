#include "config.h"
#include "wifi.h"

void netif_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    program_state_t program_state = IDLE;
    
    if (event_base == IP_EVENT || event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_CONNECTED:
                program_state = DONE;
                xQueueSend(program_state_queue, &program_state, 0);
                if (xSemaphoreTake(netif_semaphore, portMAX_DELAY) == pdTRUE) {
                    esp_netif_create_ip6_linklocal(wifi_netif);
                    xSemaphoreGive(netif_semaphore);
                }

                ESP_LOGI(TAG_WIFI, "Connected successfully");
                break;
            case IP_EVENT_TX_RX:
                program_state = PROCESSING;
                xQueueSend(program_state_queue, &program_state, 0);
                ESP_LOGI(TAG_WIFI, "IP packet received/transmitted");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                program_state = ERROR;
                xQueueSend(program_state_queue, &program_state, 0);
                ESP_LOGI(TAG_WIFI, "Connection lost");
                break;
            default:
                ESP_LOGI(TAG_WIFI, "Unknown event: %d", event_id);
                break;
        }
    }
}

