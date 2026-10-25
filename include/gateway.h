#ifndef GATEWAY_H
#define GATEWAY_H

#include "esp_netif_ip_addr.h"

#define GATEWAY_HASH_SIZE    128  // potęga 2 (min. 2x większa niż ZB_MAX_DEVICES)
#define GATEWAY_HASH_MASK    (GATEWAY_HASH_SIZE - 1) // musi być mniejsze niż 0xFF = 255

#define GATEWAY_HASH_EMPTY    0xFF // slot nigdy nieużywany (kończy wyszukiwanie)
#define GATEWAY_HASH_DELETED  0xFE // slot po usuniętym elemencie (pomijany przez wyszukiwanie)

#define GATEWAY_NAMESPACE "gateway"

typedef struct zigbee_device_s {
    bool is_active : 1;
    uint16_t not_used : 15;
    uint16_t short_addr;
    uint64_t ieee_addr;
} zigbee_device_t;

extern zigbee_device_t device_list[];

void gateway_map_init(void);

void gateway_ieee_from_ipv6(uint64_t* ieee_addr, const esp_ip6_addr_t* ipv6_addr);

void gateway_ipv6_from_ieee(esp_ip6_addr_t* ipv6_addr, const uint64_t* ieee_addr);

const zigbee_device_t* gateway_find_device_by_ieee(uint64_t ieee_addr);

const zigbee_device_t* gateway_find_device_by_short(uint16_t short_addr);

void gateway_map_update(void);

esp_err_t gateway_map_insert(uint64_t ieee_addr, uint16_t short_addr);

esp_err_t gateway_map_remove_by_ieee(uint64_t ieee_addr);

esp_err_t gateway_map_remove_by_short(uint16_t short_addr);

esp_err_t gateway_restore_map(void);

esp_err_t gateway_save_map(void);

#endif // GATEWAY_H