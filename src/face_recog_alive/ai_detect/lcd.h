/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-23 11:26:51
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-26 13:21:05
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\lcd.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>

#define DCX_GPIONUM             31// FUNC_GPIOHS31
#define RST_GPIONUM             30 // FUNC_GPIOHS30

#define SPI_CHANNEL             SPI_DEVICE_0
#define SPI_DMA_CH				DMAC_CHANNEL2
#define LCD_SPI_SLAVE_SELECT    SPI_CHIP_SELECT_3

/* clang-format off */
#define LCD_X_MAX   (240)
#define LCD_Y_MAX   (320)

#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x2f02
#define LIME       0x07E0
#define GREEN       1024
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F
#define USER_COLOR  0xAA55
#define MARINE_BLUE        0x22F
/* clang-format on */

typedef enum _lcd_dir
{
    DIR_XY_RLUD = 0x00,
    DIR_YX_RLUD = 0x20,
    DIR_XY_LRUD = 0x40,
    DIR_YX_LRUD = 0x60,
    DIR_XY_RLDU = 0x80,
    DIR_YX_RLDU = 0xA0,
    DIR_XY_LRDU = 0xC0,
    DIR_YX_LRDU = 0xE0,
    DIR_XY_MASK = 0x20,
    DIR_MASK = 0xE0,
    DIR_RGB2BRG = 0x08, // lcd_set_direction(DIR_YX_RLUD | DIR_RGB2BRG); // 0x28
} lcd_dir_t;
// typedef struct _lcd_ctl
// {
//     uint8_t mode;
//     uint8_t dir;
//     uint16_t width;
//     uint16_t height;
// } lcd_ctl_t;


void lcd_polling_enable(void);
void lcd_interrupt_enable(void);
int lcd_init(void);
void lcd_clear(uint16_t color);
void lcd_set_direction(lcd_dir_t dir);
void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_string(uint16_t x, uint16_t y, uint8_t *str, uint16_t color,uint16_t back_color);
void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color);
void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color);
void ram_draw_string(uint32_t *ptr, uint16_t x, uint16_t y, uint8_t *str, uint16_t color);
void lcd_set_rotation(char rotate);
#endif

