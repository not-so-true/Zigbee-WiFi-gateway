#include "config.h"
#include "gateway.h"
#include "zigbee.h"
#include "tools.h"

zigbee_device_t device_list[ZIGBEE_MAX_DEVICES];
static uint8_t hash_ieee[GATEWAY_HASH_SIZE];
static uint8_t hash_short[GATEWAY_HASH_SIZE];



static inline uint32_t hash_calc_for_ieee(uint64_t key) {
    key ^= (key >> 32);
    return (uint32_t)((key * 0x9E3779B1u) >> 26) & GATEWAY_HASH_MASK;
}

static inline uint32_t hash_calc_for_short(uint16_t key) {
    return (uint32_t)((uint32_t)(key * 40503u) >> 10) & GATEWAY_HASH_MASK;
}

static void hash_insert_into_ieee(uint64_t ieee_addr, uint8_t idx) {
    uint32_t hash = hash_calc_for_ieee(ieee_addr);

    for (uint32_t i = 0; i < GATEWAY_HASH_SIZE; i++) {
        uint32_t slot = (hash + i) & GATEWAY_HASH_MASK;
        if (hash_ieee[slot] == GATEWAY_HASH_EMPTY || hash_ieee[slot] == GATEWAY_HASH_DELETED) {
            hash_ieee[slot] = idx;
            return;
        }
    }
}

static void hash_insert_into_short(uint16_t short_addr, uint8_t idx) {
    uint32_t hash = hash_calc_for_short(short_addr);

    for (uint32_t i = 0; i < GATEWAY_HASH_SIZE; i++) {
        uint32_t slot = (hash + i) & GATEWAY_HASH_MASK;
        if (hash_short[slot] == GATEWAY_HASH_EMPTY || hash_short[slot] == GATEWAY_HASH_DELETED) {
            hash_short[slot] = idx;
            return;
        }
    }
}

static int hash_find_slot_in_ieee(uint64_t ieee_addr) {
    uint32_t hash = hash_calc_for_ieee(ieee_addr);

    for (uint32_t i = 0; i < GATEWAY_HASH_SIZE; i++) {
        uint32_t slot = (hash + i) & GATEWAY_HASH_MASK;
        uint8_t idx = hash_ieee[slot];

        if (idx == GATEWAY_HASH_EMPTY)
            return -1; // koniec, nie ma elementu

        if (idx != GATEWAY_HASH_DELETED && device_list[idx].ieee_addr == ieee_addr)
            return (int) slot;
    }
    
    return -1;
}

static int hash_find_slot_in_short(uint16_t short_addr) {
    uint32_t hash = hash_calc_for_short(short_addr);
    
    for (uint32_t i = 0; i < GATEWAY_HASH_SIZE; i++) {
        uint32_t slot = (hash + i) & GATEWAY_HASH_MASK;
        uint8_t idx = hash_short[slot];

        if (idx == GATEWAY_HASH_EMPTY)
            return -1; // koniec, nie ma elementu
        
        if (idx != GATEWAY_HASH_DELETED && device_list[idx].short_addr == short_addr)
            return (int)slot;
    }

    return -1;
}



void gateway_map_init(void) {
    memset(device_list, 0, sizeof(device_list));
    memset(hash_ieee, GATEWAY_HASH_EMPTY, sizeof(hash_ieee));
    memset(hash_short, GATEWAY_HASH_EMPTY, sizeof(hash_short));
}

void gateway_ieee_from_ipv6(uint64_t* ieee_addr, const esp_ip6_addr_t* ipv6_addr) {
    *ieee_addr = __ntohl(((uint64_t) ipv6_addr->addr[2] ^ 0x02000000) << 32);
    *ieee_addr = __ntohl(*ieee_addr | ipv6_addr->addr[3]);
}

void gateway_ipv6_from_ieee(esp_ip6_addr_t* ipv6_addr, const uint64_t* ieee_addr) {
    ipv6_addr->addr[0] = __htonl(0xfd95a8be);
    ipv6_addr->addr[1] = __htonl(0x83b25788);
    ipv6_addr->addr[2] = __htonl(((uint32_t) (*ieee_addr >> 32)) ^ 0x02000000);
    ipv6_addr->addr[3] = __htonl((uint32_t) (*ieee_addr));
}

const zigbee_device_t* gateway_find_device_by_ieee(uint64_t ieee_addr) {
    int slot = hash_find_slot_in_ieee(ieee_addr);
    return (slot >= 0) ? &device_list[hash_ieee[slot]] : NULL;
}

const zigbee_device_t* gateway_find_device_by_short(uint16_t short_addr) {
    int slot = hash_find_slot_in_short(short_addr);
    return (slot >= 0) ? &device_list[hash_short[slot]] : NULL;
}

void gateway_map_update(void) {
    
}

esp_err_t gateway_map_insert(uint64_t ieee_addr, uint16_t short_addr) {
    // 1. sprawdzenie czy urządzenie o tym IEEE już jest w bazie
    int ieee_slot = hash_find_slot_in_ieee(ieee_addr);
    if (ieee_slot >= 0) {
        uint8_t idx = hash_ieee[ieee_slot];
        
        if (device_list[idx].short_addr != short_addr) {
            int old_short_slot = hash_find_slot_in_short(device_list[idx].short_addr);
            if (old_short_slot >= 0) {
                hash_short[old_short_slot] = GATEWAY_HASH_DELETED;
            }
            device_list[idx].short_addr = short_addr;
            hash_insert_into_short(short_addr, idx);
        }
        return ESP_OK;
    }

    // 2. szukanie wolnego slotu w device_list
    uint8_t free_idx = GATEWAY_HASH_EMPTY;
    for (uint8_t i = 0; i < ZIGBEE_MAX_DEVICES; i++) {
        if (!device_list[i].is_active) {
            free_idx = i;
            break;
        }
    }

    // osiągnięto limit urządzeń
    if (free_idx == GATEWAY_HASH_EMPTY) return ESP_ERR_NOT_ALLOWED;

    // 3. update danych na liśce i indeksu w obu tablicach hash
    device_list[free_idx].ieee_addr = ieee_addr;
    device_list[free_idx].short_addr = short_addr;
    device_list[free_idx].is_active = true;

    hash_insert_into_ieee(ieee_addr, free_idx);
    hash_insert_into_short(short_addr, free_idx);

    return ESP_OK;
}

esp_err_t gateway_map_remove_by_ieee(uint64_t ieee_addr) {
    int ieee_slot = hash_find_slot_in_ieee(ieee_addr);
    if (ieee_slot < 0) return ESP_ERR_NOT_FOUND;

    uint8_t idx = hash_ieee[ieee_slot];

    // oznaczenie miejsca na liście jako deleted
    hash_ieee[ieee_slot] = GATEWAY_HASH_DELETED;

    int short_slot = hash_find_slot_in_short(device_list[idx].short_addr);
    if (short_slot >= 0)
        hash_short[short_slot] = GATEWAY_HASH_DELETED;

    // oznaczenie urządzenia jako usunięte
    device_list[idx].ieee_addr = 0;
    device_list[idx].short_addr = 0;
    device_list[idx].is_active = false;

    return ESP_OK;
}

esp_err_t gateway_map_remove_by_short(uint16_t short_addr) {
    int short_slot = hash_find_slot_in_short(short_addr);
    if (short_slot < 0) return ESP_ERR_NOT_FOUND;

    uint8_t idx = hash_short[short_slot];
    return gateway_map_remove_by_ieee(device_list[idx].ieee_addr);
}

esp_err_t gateway_restore_map(void) {
    esp_err_t err = ESP_OK;
    gateway_map_init();

    size_t size = sizeof(device_list);
    err = tools_read_from_nvs(GATEWAY_NAMESPACE, "device_list", device_list, &size);
    if (err != ESP_OK) return err;

    gateway_map_update();
    
    return err;
}

esp_err_t gateway_save_map(void) {
    esp_err_t err = ESP_OK;

    size_t size = sizeof(device_list);
    err = tools_save_to_nvs(GATEWAY_NAMESPACE, "device_list", device_list, &size);

    return err;
}