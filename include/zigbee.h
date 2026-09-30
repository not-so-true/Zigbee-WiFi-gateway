#ifndef ZIGBEE_H
#define ZIGBEE_H

#include "esp_zigbee.h"
#include "ezbee/zha.h"


#define ZGB_CHANNELS_TO_SCAN ((1UL << 15) | (1UL << 20) | (1UL << 25))

void zigbee_stack_task(void* pvParameters);
void zigbee_cluster_handler(ezb_zcl_core_action_callback_id_t callback_id, void *message);
bool zigbee_handler(const ezb_app_signal_t* signal);
esp_err_t zigbee_init_descriptors(void);

#endif // ZIGBEE_H