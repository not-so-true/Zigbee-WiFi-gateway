#include "config.h"
#include "tools.h"
#include "wifi.h"
#include "zigbee.h"

QueueHandle_t program_state_queue;
TimerHandle_t program_state_timer;

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

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led));

    ESP_LOGI(TAG_LED, "Initialized successfully");
}

void wifi_init(void) {
    wifi_netif = esp_netif_create_default_wifi_sta();
    netif_semaphore = xSemaphoreCreateMutex();

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
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_handler, NULL, NULL);
    
    ESP_LOGI(TAG_WIFI, "Initialized successfully");
}

void zigbee_init(void) {
    esp_zb_platform_config_t zgb_modem_cfg = {
        .radio_config = {.radio_mode = ZB_RADIO_MODE_NATIVE}, // external or internal radio
        .host_config = {.host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE} // external or internal commands
    };
    esp_zb_cfg_t zgb_network_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,
        .install_code_policy = false, // security setting, for later considaration
        .nwk_cfg.zczr_cfg = {.max_children = 32} // directly connected devices (other through routers)
    };

    esp_zb_platform_config(&zgb_modem_cfg);
    esp_zb_init(&zgb_network_cfg);

    esp_zb_endpoint_config_t zgb_endpoint_cfg = {
        .endpoint = 1,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_HOME_GATEWAY_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();

    esp_zb_cluster_list_add_basic_cluster(
        cluster_list, 
        esp_zb_basic_cluster_create(NULL), 
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE
    );
    esp_zb_cluster_list_add_identify_cluster(
        cluster_list, 
        esp_zb_identify_cluster_create(NULL), 
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE
    );
    
    esp_zb_ep_list_add_ep(ep_list, cluster_list, zgb_endpoint_cfg);
    esp_zb_device_register(ep_list);

    esp_zb_set_primary_network_channel_set(ZGB_CHANNELS_TO_SCAN);

    esp_zb_core_action_handler_register(zigbee_handler);

    ESP_ERROR_CHECK(esp_zb_start(false));

    ESP_LOGI(TAG_ZIGBEE, "Initialized successfully");
}

void app_main(void) {   
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    led_init();
    wifi_init();
    zigbee_init();

    // led startup
    program_state_queue = xQueueCreate(3, sizeof(program_state_t));
    program_state_timer = xTimerCreate(
        "program_state_timer",
        pdMS_TO_TICKS(5000),
        pdFALSE,
        NULL,
        program_state_callback
    );
    xTaskCreate(led_blink_task, "led_blink", 1024, led, 2, NULL);

    // wifi startup
    post_program_state(PROCESSING);
    esp_wifi_connect();

    // zigbee startup
    xTaskCreate(zgb_stack_task, "zgb_stack", 8192, NULL, 20, NULL);

    esp_netif_ip_info_t ipv4;
    esp_ip6_addr_t ipv6[CONFIG_LWIP_IPV6_NUM_ADDRESSES];
    vTaskDelay(pdMS_TO_TICKS(20000));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
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
        ESP_LOGI(TAG_WIFI, "IPv6 jakieś: " IPV6STR, IPV62STR(ipv6[2]));
    }
}