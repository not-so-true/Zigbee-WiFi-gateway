#include "config.h"
#include "zigbee.h"

void zgb_stack_task(void* pvParameters) {
    esp_zb_stack_main_loop();
    vTaskDelete(NULL);
}

esp_err_t zigbee_handler(esp_zb_core_action_callback_id_t callback_id, const void *message) {
    // ...
    return ESP_OK;
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    // ...
}