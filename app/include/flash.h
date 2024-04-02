#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "device_config.h"
//每一次大的更改配置结构的时候建议修改版本
#define CFG_VERSION (0.2f)
#define CFG_HEADER (0x55AA5558)

typedef struct
{
    uint32_t stat; //0,rgb; 1,ir; 2,ir+rgb
    int8_t fea_rgb[196];
    int8_t fea_ir[196];
} face_fea_t;

typedef struct _face_info_t
{
    uint32_t index;
    uint8_t uid[UID_LEN];
    uint32_t valid;
    // float info[196];
    face_fea_t info;
    char name[NAME_LEN];
    char note[NOTE_LEN];
} face_info_t;

typedef struct _face_save_info_t
{
    uint32_t header;
    uint32_t version;
    uint32_t number;
    uint32_t checksum;
    uint32_t face_info_index[FACE_DATA_MAX_COUNT / 32 + 1];
} face_save_info_t __attribute__((aligned(8)));

// 定义config_data_t结构体
typedef struct {
    bool seatSwitch;
    int seatLevel;
    int seatInterval;
    float seatVoltage;

    bool accSwitch;
    int accInterval;
    float accVoltage;
    
    bool relaySwitch;

    bool canControlSwitch;
    int canType;
    bool canBreakSwitch;
    int canBreakInterval;
    
    float recognitionThreshold;

    uint8_t wifi_ssid[32];
    uint8_t wifi_passwd[32];

} config_data_t;

extern volatile config_data_t g_config;
extern volatile face_save_info_t g_face_save_info;

bool flash_init(void);
uint64_t	get_chip_number(void);

#endif // !_FLASH_H