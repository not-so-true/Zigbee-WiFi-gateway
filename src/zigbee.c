#include "config.h"
#include "zigbee.h"
#include "tools.h"

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
    
    //ezb_aps_secur_enable_distributed_security(false);
    ezb_bdb_set_primary_channel_set(ZIGBEE_PRIMARY_CHANNELS);

    ESP_LOGI(TAG_ZIGBEE, "Initialized successfully");
}

void zigbee_stack_task(void* pvParameters) {
    zigbee_init();

    post_program_state(PROCESSING);
    esp_zigbee_start(false);

    esp_zigbee_launch_mainloop();

    esp_zigbee_deinit();
    vTaskDelete(NULL);
}

esp_err_t zigbee_init_descriptors(void) {
    ezb_zcl_basic_cluster_server_config_t basic_cfg = {
        .zcl_version = EZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = EZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE
    };

    // klastry serwerowe
    ezb_zcl_cluster_desc_t zcl_basic = ezb_zcl_basic_create_cluster_desc(&basic_cfg, EZB_ZCL_CLUSTER_SERVER);
    ezb_zcl_cluster_desc_t zcl_identify = ezb_zcl_identify_create_cluster_desc(NULL, EZB_ZCL_CLUSTER_SERVER);
    ezb_zcl_cluster_desc_t zcl_time = ezb_zcl_time_create_cluster_desc(NULL, EZB_ZCL_CLUSTER_SERVER);
    
    ezb_zcl_basic_server_generic_device_type_t generic_device_type = EZB_ZCL_BASIC_GENERIC_DEVICE_TYPE_GATEWAY_OR_BRIDGE;
    ezb_zcl_basic_cluster_desc_add_attr(zcl_basic, EZB_ZCL_ATTR_BASIC_GENERIC_DEVICE_TYPE_ID, &generic_device_type);

    // klastry klienckie - brak, ze względu na gateway_endpoint, który pomija weryfikację klastrów klienckich w wymianie danych
    // ezb_zcl_cluster_desc_t zcl_basic_c = ezb_zcl_basic_create_cluster_desc(NULL, EZB_ZCL_CLUSTER_CLIENT);
    // ezb_zcl_cluster_desc_t zcl_identify_c = ezb_zcl_identify_create_cluster_desc(NULL, EZB_ZCL_CLUSTER_CLIENT);

    ezb_af_ep_config_t endpoint_cfg = {
        .ep_id = 1,
        .app_profile_id = EZB_AF_HA_PROFILE_ID,
        .app_device_id = EZB_ZHA_HOME_GATEWAY_DEVICE_ID,
        .app_device_version = 0
    };

    ezb_af_ep_desc_t zcl_endpoint = ezb_af_create_gateway_endpoint(&endpoint_cfg);
    if (zcl_endpoint == EZB_INVALID_AF_EP_DESC)
        return ESP_ERR_NOT_FINISHED;
    
    ezb_af_endpoint_add_cluster_desc(zcl_endpoint, zcl_basic);
    ezb_af_endpoint_add_cluster_desc(zcl_endpoint, zcl_identify);
    ezb_af_endpoint_add_cluster_desc(zcl_endpoint, zcl_time);
        
    ezb_af_device_desc_t zcl_node = ezb_af_create_device_desc();
    if (zcl_node == EZB_INVALID_AF_DEVICE_DESC)
        return ESP_ERR_NOT_FINISHED;
    
    ezb_err_t err1 = ezb_af_device_add_endpoint_desc(zcl_node, zcl_endpoint);
    ezb_err_t err2 = ezb_af_device_desc_register(zcl_node);

    if (err1 != EZB_ERR_NONE || err2 != EZB_ERR_NONE)
        return ESP_ERR_NOT_FINISHED;

    return ESP_OK;
}

bool zigbee_handler(const ezb_app_signal_t* signal) {
    ezb_app_signal_type_t signal_type = ezb_app_signal_get_type(signal);
    ezb_err_t err = EZB_ERR_NONE;

    switch (signal_type) {
    case EZB_ZDO_SIGNAL_SKIP_STARTUP: {
        err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_INITIALIZATION);
        break;
    }
    case EZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case EZB_BDB_SIGNAL_DEVICE_REBOOT: {
        ezb_bdb_signal_simple_params_t params = *((ezb_bdb_signal_simple_params_t *)ezb_app_signal_get_params(signal));
        if (params.status == EZB_BDB_STATUS_SUCCESS) {
            if (ezb_bdb_is_factory_new()) {
                ESP_LOGI(TAG_ZIGBEE, "Factory new device, creating network...");
                err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_NETWORK_FORMATION);
            } else {
                ESP_LOGI(TAG_ZIGBEE, "Device reboot, network recreated");
                post_program_state(DONE);

                zigbee_print_network_info();

                xEventGroupSetBits(event_group, ZIGBEE_BIT);
            }
        } else {
            ESP_LOGE(TAG_ZIGBEE, "Network initialisation failure (status: 0x%02x), retrying...", params.status);
            post_program_state(ERROR);

            err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_INITIALIZATION);
        }
        break;
    }
    case EZB_BDB_SIGNAL_FORMATION: { // formacja sieci przy ustawieniach fabrycznych (pierwszy start)
        ezb_bdb_signal_simple_params_t params = *((ezb_bdb_signal_simple_params_t *)ezb_app_signal_get_params(signal));
        if (params.status == EZB_BDB_STATUS_SUCCESS) {
            ESP_LOGI(TAG_ZIGBEE, "Network creation successful");
            post_program_state(DONE);

            zigbee_print_network_info();

            xEventGroupSetBits(event_group, ZIGBEE_BIT);
        } else {
            ESP_LOGE(TAG_ZIGBEE, "Network creation failure (status: 0x%02x), retrying...", params.status);
            post_program_state(ERROR);

            err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_NETWORK_FORMATION);
        }
        break;
    }
    case EZB_BDB_SIGNAL_STEERING: {
        ezb_bdb_signal_simple_params_t params = *((ezb_bdb_signal_simple_params_t *)ezb_app_signal_get_params(signal));

        if (params.status != EZB_BDB_STATUS_SUCCESS) {
            ESP_LOGE(TAG_ZIGBEE, "Network steering failure (status: 0x%02x), retrying...", params.status);
            post_program_state(ERROR);

            err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_NETWORK_STEERING);
        }
        break;
    }


    case EZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED: {
        break;
    }
    case EZB_NWK_SIGNAL_NETWORK_STATUS: { // statusy błędów zwracane w sieci
        ezb_nwk_signal_network_status_params_t params = *((ezb_nwk_signal_network_status_params_t *)ezb_app_signal_get_params(signal));
        ezb_extaddr_t extaddr;
        err = ezb_address_extended_by_short(params.network_addr, &extaddr); // inny error handling!
        ESP_LOGE(TAG_ZIGBEE, "Device 0x%04x reported %s", params.network_addr, ezb_nwk_network_status_to_string(params.status));
        break;
    }
    case EZB_NWK_SIGNAL_PERMIT_JOIN_STATUS: { // informacja o zamknieciu lub otwarciu sieci
        ezb_nwk_signal_permit_join_status_params_t params = *((ezb_nwk_signal_permit_join_status_params_t *)ezb_app_signal_get_params(signal));
        if (params.duration > 0) {
            ESP_LOGI(TAG_ZIGBEE, "Network open for %d s", params.duration);
            post_program_state(PROCESSING);
        } else {
            ESP_LOGI(TAG_ZIGBEE, "Network pairing closed");
            post_program_state(IDLE);
        }
        break;
    }


    case EZB_ZDO_SIGNAL_DEVICE_ANNCE: { // rejestracja urządzenia w sieci
        ezb_zdo_signal_device_annce_params_t params = *((ezb_zdo_signal_device_annce_params_t *)ezb_app_signal_get_params(signal));
        ESP_LOGI(TAG_ZIGBEE, "Device joined the network -> short addr: 0x%04X | IEEE addr: 0x%016llX", params.short_addr, params.device_addr.u64);
        post_program_state(DONE);

        // tu docelowo: rejestracja urządzenia

        break;
    }
    case EZB_ZDO_SIGNAL_DEVICE_UNAVAILABLE: { // urządzenie nieosiągalne
        ezb_zdo_signal_device_unavailable_params_t params = *((ezb_zdo_signal_device_unavailable_params_t *)ezb_app_signal_get_params(signal));
        ESP_LOGI(TAG_ZIGBEE, "Device 0x%016llx unreachable", params.device_addr.u64);
        break;
    }
    case EZB_ZDO_SIGNAL_LEAVE_INDICATION: { // powiadomienie o wyjściu urządzenia z sieci
        ezb_zdo_signal_leave_indication_params_t params = *((ezb_zdo_signal_leave_indication_params_t *)ezb_app_signal_get_params(signal));
        if (params.leave_type == EZB_ZDO_LEAVE_TYPE_REJOIN)
            ESP_LOGI(TAG_ZIGBEE, "Device 0x%016llx left the network, will try to rejoin", params.device_addr.u64);
        else
            ESP_LOGI(TAG_ZIGBEE, "Device 0x%016llx left the network", params.device_addr.u64);
        break;
    }
    case EZB_ZDO_SIGNAL_LEAVE: { // wyjście bramki z sieci
        ezb_zdo_signal_leave_params_t params = *((ezb_zdo_signal_leave_params_t *)ezb_app_signal_get_params(signal));
        if (params.leave_type == EZB_ZDO_LEAVE_TYPE_REJOIN)
            ESP_LOGI(TAG_ZIGBEE, "Left the network, will try to rejoin");
        else
            ESP_LOGI(TAG_ZIGBEE, "Left the network");
        break;
    }
    case EZB_ZDO_SIGNAL_ERROR: {
        ESP_LOGE(TAG_ZIGBEE, "Generic error in signaling detected");
        break;
    }
    default:
        ESP_LOGW(TAG_ZIGBEE, "Unhandled signal received: %s", ezb_app_signal_to_string(signal_type));
        return false;
        break;
    }

    if (err != EZB_ERR_NONE) {
        ESP_LOGE(TAG_ZIGBEE, "Error at %s, code: 0x%02X", ezb_app_signal_to_string(signal_type), err);
        post_program_state(ERROR);
    }

    return true;
}

void zigbee_cluster_handler(ezb_zcl_core_action_callback_id_t callback_id, void *message) {
    // ...
}

esp_err_t zigbee_start_pairing(void) {
    ezb_err_t err;

    esp_zigbee_lock_acquire(portMAX_DELAY);
    err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_NETWORK_STEERING);
    esp_zigbee_lock_release();

    if (err != EZB_ERR_NONE)
        return ESP_FAIL; // start parowania nieudany

    return ESP_OK;
}

void zigbee_print_network_info(void) {
    esp_zigbee_lock_acquire(portMAX_DELAY);

    uint8_t channel = ezb_nwk_get_current_channel();
    ezb_panid_t panid = ezb_nwk_get_panid();
    ezb_shortaddr_t shortaddr = ezb_nwk_get_short_address();

    ezb_extaddr_t extaddr;
    ezb_nwk_get_extended_address(&extaddr);
    ezb_extpanid_t extpanid;
    ezb_nwk_get_extended_panid(&extpanid);

    esp_zigbee_lock_release();

    ESP_LOGI(TAG_ZIGBEE, "===== ZIGBEE NETWORK INFO =====");
    ESP_LOGI(TAG_ZIGBEE, "Channel:     %d", channel);
    ESP_LOGI(TAG_ZIGBEE, "PAN ID:      0x%04X", panid);
    ESP_LOGI(TAG_ZIGBEE, "Ext PAN ID:  0x%016llX", extpanid.u64);
    ESP_LOGI(TAG_ZIGBEE, "Coord Addr:  0x%04X", shortaddr);
    ESP_LOGI(TAG_ZIGBEE, "Coord MAC:   0x%016llX", extaddr.u64);
    ESP_LOGI(TAG_ZIGBEE, "===============================");
}

void zigbee_print_active_devices(void) {
    esp_zigbee_lock_acquire(portMAX_DELAY);

    // ezb_nwk_nbr_iterator_t it = EZB_NWK_NBR_ITERATOR_INITIALIZER;
    // ezb_nwk_nbr_record_t record;
    // uint16_t count = 0;

    // ESP_LOGI(TAG_ZIGBEE, "=== CONNECTED DEVICES ===");
    // while (ezb_nwk_nbr_iterator_next(&it, &record) == EZB_ERR_NONE) {
    //     const char *role = (record.device_type == EZB_NWK_DEVICE_TYPE_ROUTER) ? "Router" :
    //                        (record.device_type == EZB_NWK_DEVICE_TYPE_END_DEVICE) ? "EndDev" : "Other";

    //     ESP_LOGI(TAG_ZIGBEE, "[%d] Short: 0x%04X | IEEE: 0x%016llX | Role: %s | LQI: %d | RSSI: %d dBm",
    //              ++count, record.short_addr, record.ieee_addr.u64, role, record.lqi, record.rssi);
    // }

    // if (count == 0) {
    //     ESP_LOGI(TAG_ZIGBEE, "No devices connected.");
    // }

    esp_zigbee_lock_release();
}