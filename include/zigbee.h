#ifndef ZIGBEE_H
#define ZIGBEE_H

#include "esp_zigbee.h"
#include "ezbee/zha.h"

#define ZIGBEE_PRIMARY_CHANNELS ((1UL << 17) | (1UL << 18))

void zigbee_init(void);
void zigbee_stack_task(void* pvParameters);
void zigbee_cluster_handler(ezb_zcl_core_action_callback_id_t callback_id, void *message);
bool zigbee_handler(const ezb_app_signal_t* signal);
esp_err_t zigbee_init_descriptors(void);

#endif // ZIGBEE_H