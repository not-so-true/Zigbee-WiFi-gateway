#ifndef WIFI_H
#define WIFI_H

#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

// #define WIFI_SSID "Kamil's Galaxy S23+"
// #define WIFI_PASS "1234abcd"
#define WIFI_SSID "Network 2 2,4G"
#define WIFI_PASS "Kamil2004"
// #define WIFI_SSID "Honor Magic V6"
// #define WIFI_PASS "1234abcd"

extern esp_netif_t* wifi_netif;
extern SemaphoreHandle_t netif_semaphore;

void wifi_init(void);

void wifi_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void wifi_print_network_info(void);

#endif // WIFI_H