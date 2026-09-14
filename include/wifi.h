#ifndef WIFI_H
#define WIFI_H

#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define WIFI_SSID "Kamil's Galaxy S23+"
#define WIFI_PASS "1234abcd"
//#define WIFI_SSID "Network 2 2,4G"
//#define WIFI_PASS "Kamil2004"

extern esp_netif_t* wifi_netif;
extern SemaphoreHandle_t netif_semaphore;

void netif_handler(void* arg, 
                    esp_event_base_t event_base, 
                    int32_t event_id, 
                    void* event_data);

#endif // WIFI_H