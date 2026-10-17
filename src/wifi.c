#include "config.h"
#include "wifi.h"
#include "zigbee.h"
#include "tools.h"

esp_netif_t* wifi_netif;

void wifi_init(void) {
    wifi_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .failure_retry_cnt = 4
        },
    };

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_handler, NULL, NULL);
    
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "Initialized successfully");
}

void wifi_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch (event_id) {
    case WIFI_EVENT_STA_START: {
        post_program_state(PROCESSING);
        esp_wifi_connect();
        break;
    }
    case WIFI_EVENT_STA_CONNECTED: {
        post_program_state(DONE);
        esp_netif_create_ip6_linklocal(wifi_netif);

        wifi_event_sta_connected_t sta_connected = *((wifi_event_sta_connected_t*) event_data);
        ESP_LOGI(TAG_WIFI, "Connected successfully to \"%s\"", sta_connected.ssid);
        break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
        post_program_state(ERROR);
        wifi_event_sta_disconnected_t sta_disconnected = *((wifi_event_sta_disconnected_t*) event_data);
        ESP_LOGI(TAG_WIFI, "Connection with \"%s\" lost, reason: %d, retrying...", sta_disconnected.ssid, sta_disconnected.reason);

        esp_wifi_connect();
        break;
    }
    default:
        ESP_LOGW(TAG_WIFI, "Unknown event: %d", event_id);
        break;
    }
}

void ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
        esp_netif_ip_info_t ipv4;
        esp_netif_get_ip_info(wifi_netif, &ipv4);

        ESP_LOGI(TAG_WIFI, "Adres IP: " IPSTR, IP2STR(&ipv4.ip));
        ESP_LOGI(TAG_WIFI, "Maska:    " IPSTR, IP2STR(&ipv4.netmask));
        ESP_LOGI(TAG_WIFI, "Brama:    " IPSTR, IP2STR(&ipv4.gw));
        break;
    }
    case IP_EVENT_TX_RX: {
        ESP_LOGI(TAG_WIFI, "IP packet received/transmitted");
        break;
    }
    default:
        ESP_LOGW(TAG_WIFI, "Unknown event: %d", event_id);
        break;
    }
}