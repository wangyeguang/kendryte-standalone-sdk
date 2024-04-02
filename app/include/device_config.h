/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-02 10:37:12
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-04-02 13:15:24
 * @FilePath: \kendryte-standalone-sdk\app\include\device_config.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _DEVICE_CONFIG_H
#define _DEVICE_CONFIG_H

#define LOCK_IN_SECTION(s)                          __attribute__((used,unused,section(".iodata." #s)))


#define CONFIG_TOOLCHAIN_PATH "/opt/kendryte-toolchain/bin"
#define CONFIG_TOOLCHAIN_PREFIX "riscv64-unknown-elf-"
#define CONFIG_BOARD_MAIX 1
#define CONFIG_LCD_DEFAULT_WIDTH 320
#define CONFIG_LCD_DEFAULT_HEIGHT 240
#define CONFIG_LCD_DEFAULT_FREQ 15000000
#define CONFIG_SENSOR_FREQ 24000000
#define CONFIG_CPU_DEFAULT_FREQ 400000000
#define CONFIG_COMPONENT_DRIVERS_ENABLE 1
#define CONFIG_COMPONENT_KENDRYTE_SDK_ENABLE 1
#define CONFIG_SDK_LOG_LEVEL 5
#define CONFIG_SDK_LOCK_NUM 64
#define CONFIG_FREERTOS_ENABLE 1
#define CONFIG_STATIC_TASK_CLEAN_UP_ENABLE 1
#define CONFIG_FREEROTS_MINIMUM_STACK_SIZE 2048

#define FREQ_PLL0_MAX        1200000000UL //1800MHz max
#define FREQ_PLL0_DEFAULT    (CONFIG_CPU_DEFAULT_FREQ*2)
#define FREQ_PLL0_MIN        52000000UL
#define FREQ_PLL1_MAX        1200000000UL //1800MHz max
#define FREQ_PLL1_DEFAULT    400000000UL
#define FREQ_PLL1_MIN        26000000UL
#define FREQ_PLL2_DEFAULT    45158400UL

#define FREQ_CPU_MAX     600000000UL
#define FREQ_CPU_DEFAULT (CONFIG_CPU_DEFAULT_FREQ)
#define FREQ_CPU_MIN     (FREQ_PLL0_MIN/2)
#define FREQ_KPU_MAX     600000000UL
#define FREQ_KPU_DEFAULT FREQ_PLL1_DEFAULT
#define FREQ_KPU_MIN     FREQ_PLL1_MIN

//////////////////////////////////////////////////////
//flash addr
#define	OTP_MANUFACTURE_DATA_ADDR	0x3D90
#define KMODEL_FACE_DETECT_ADDR 0x300000
#define KMODEL_FACE_DETECT_SIZE 278440
#define KMODEL_FACE_LD_ADDR 0x400000
#define KMODEL_FACE_LD_SIZE 159768
#define KMODEL_FACE_RECOGNITION_ADDR 0x500000
#define KMODEL_FACE_RECOGNITION_SIZE 1108368
#define FONT_DATA_ADDR 0x700000
#define FONT_DATA_SIZE 2097152

#define CONFIG_DATA_ADDR 0x910000 //9M+64K
//人臉特征值地址 2M
#define FACE_FEATURE_ADDR 0x920000 //9M+128k
#define FACE_HEADER                                 (0x55AA5503)
#define FACE_DATA_MAX_COUNT                         (512)
//人臉識別記錄地址
#define FACE_RECORD_ADDR 0xb00000 //11M

#define PIC_DATA_ADDR 0xd00000 //13M

#define UID_LEN                                     (16)
#define NAME_LEN                                    (16)
#define NOTE_LEN                                    (16)

#endif // !_DEVICE_CONFIG_H

