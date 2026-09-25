#ifndef ZIGBEE_H
#define ZIGBEE_H

#include "esp_zigbee_core.h"

#define ZGB_CHANNELS_TO_SCAN ((1UL << 15) | (1UL << 20) | (1UL << 25))

void zgb_stack_task(void* pvParameters);
esp_err_t zigbee_handler(esp_zb_core_action_callback_id_t callback_id, const void *message);
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct);

#endif // ZIGBEE_H