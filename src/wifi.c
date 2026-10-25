#include "config.h"
#include "wifi.h"
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
        program_state_post(PROCESSING);
        esp_wifi_connect();
        break;
    }
    case WIFI_EVENT_STA_CONNECTED: {
        program_state_post(DONE);
        esp_netif_create_ip6_linklocal(wifi_netif);

        wifi_event_sta_connected_t sta_connected = *((wifi_event_sta_connected_t*) event_data);
        ESP_LOGI(TAG_WIFI, "Connected successfully to \"%s\"", sta_connected.ssid);
        break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
        program_state_post(ERROR);
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
        xEventGroupSetBits(event_group, WIFI_BIT);
        break;
    }
    case IP_EVENT_GOT_IP6: {
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

void wifi_print_network_info(void) {
    wifi_ap_record_t ap_info;
    esp_netif_ip_info_t ipv4 = {0};
    esp_ip6_addr_t ipv6[CONFIG_LWIP_IPV6_NUM_ADDRESSES] = {0};
    uint8_t mac[6] = {0};

    esp_wifi_get_mac(WIFI_IF_STA, mac);
    esp_netif_get_ip_info(wifi_netif, &ipv4);
    int ipv6_count = esp_netif_get_all_ip6(wifi_netif, ipv6);

    ESP_LOGI(TAG_WIFI, "========= NETWORK INFO ========");
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        ESP_LOGI(TAG_WIFI, "Channel:     %d", ap_info.primary);
        ESP_LOGI(TAG_WIFI, "SSID:        %s", ap_info.ssid);
        ESP_LOGI(TAG_WIFI, "RSSI:        %d dBm", ap_info.rssi);
        ESP_LOGI(TAG_WIFI, "IPv4 addr:   " IPSTR, IP2STR(&ipv4.ip));
        ESP_LOGI(TAG_WIFI, "Netmask:     " IPSTR, IP2STR(&ipv4.netmask));
        ESP_LOGI(TAG_WIFI, "Gateway:     " IPSTR, IP2STR(&ipv4.gw));
        for (int i = 1; i < ipv6_count+1; i++)
            ESP_LOGI(TAG_WIFI, "IPv6 addr %d: " IPV6STR, i, IPV62STR(ipv6[i-1]));
    } else
        ESP_LOGW(TAG_WIFI, "Status:      Not connected");

    ESP_LOGI(TAG_WIFI, "MAC:         %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG_WIFI, "===============================");
}
