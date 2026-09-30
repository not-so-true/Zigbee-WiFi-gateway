#include "config.h"
#include "zigbee.h"

void zigbee_stack_task(void* pvParameters) {
    esp_zigbee_launch_mainloop();
    esp_zigbee_deinit();
    vTaskDelete(NULL);
}

void zigbee_cluster_handler(ezb_zcl_core_action_callback_id_t callback_id, void *message) {
    // ...
}

bool zigbee_handler(const ezb_app_signal_t* signal) {
    //ezb_app_signal_get_params();
    //ezb_app_signal_get_type();
    return true;
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
