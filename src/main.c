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
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB
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
    ESP_ERROR_CHECK(nvs_flash_init_partition("zb_storage"));

    esp_zigbee_device_config_t zigbee_role = {
        .device_type = EZB_NWK_DEVICE_TYPE_COORDINATOR,
        .install_code_policy = false, // security setting, for later considaration
        .zczr_config = {.max_children = 32} // directly connected devices (other through routers)
    };
    esp_zigbee_platform_config_t zigbee_modem = {
        .storage_partition_name = "zb_storage",
        .radio_config = {.radio_mode = ESP_ZIGBEE_RADIO_MODE_NATIVE}, // external or internal radio
    };
    esp_zigbee_config_t zigbee_cfg = {
        .device_config = zigbee_role,
        .platform_config = zigbee_modem
    };

    ESP_ERROR_CHECK(esp_zigbee_init(&zigbee_cfg));
    ESP_ERROR_CHECK(zigbee_init_descriptors());

    ezb_app_signal_add_handler(zigbee_handler);
    ezb_zcl_core_action_handler_register(zigbee_cluster_handler);
    
    // ezb_mem_config_t zigbee_memory_cfg = {
    //         /** The capacity of the buffer pool */
    //     .buffer_pool_size = ,
    //     /** The capacity of the address table */
    //     .address_table_size = ,
    //     /** The capacity of the neighbor table */
    //     .neighbor_table_size = ,
    //     /** The capacity of the route table */
    //     .route_table_size = ,
    //     /** The capacity of the route discovery table */
    //     .route_discovery_table_size = ,
    //     /** The capacity of the route record table */
    //     .route_record_table_size = ,
    //     /** The capacity of the APS device key pair set */
    //     .aps_key_pair_set_size = ,
    //     /** The capacity of source entries in the APS binding table */
    //     .aps_bind_table_src_size = ,
    //     /** The capacity of destination entries in the APS binding table */
    //     .aps_bind_table_dst_size = 
    // };
    // ezb_config_memory(&zigbee_memory_cfg);

    ezb_bdb_set_primary_channel_set(ZGB_CHANNELS_TO_SCAN);

    ESP_LOGI(TAG_ZIGBEE, "Initialized successfully");
}

void app_main(void) {   
    vTaskDelay(pdMS_TO_TICKS(4000));

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
    post_program_state(PROCESSING);
    esp_zigbee_start(false);
    xTaskCreate(zigbee_stack_task, "zigbee_stack", 8192, NULL, 20, NULL);

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