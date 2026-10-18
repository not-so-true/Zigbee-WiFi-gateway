#ifndef ZIGBEE_H
#define ZIGBEE_H

#include "esp_zigbee.h"
#include "ezbee/zha.h"

#define ZIGBEE_PRIMARY_CHANNELS ((1UL << 17) | (1UL << 18))

void zigbee_init(void);

void zigbee_stack_task(void* pvParameters);

esp_err_t zigbee_init_descriptors(void);

bool zigbee_handler(const ezb_app_signal_t* signal);

void zigbee_cluster_handler(ezb_zcl_core_action_callback_id_t callback_id, void *message);

esp_err_t zigbee_start_pairing(void);

void zigbee_print_network_info(void);

void zigbee_print_active_devices(void);

#endif // ZIGBEE_H