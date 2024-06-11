/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-02 16:23:22
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-06 17:56:33
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\lcd.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "lcd.h"
#include "sleep.h"
#include "fpioa.h"
#include "sysctl.h"
#include "st7789.h"
#include<spi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sleep.h"
#include "font.h"
#include "w25qxx.h"
#include "device_config.h"

#define LCD_SWAP_COLOR_BYTES 1
#define SWAP_16(x) ((x >> 8 & 0xff) | (x << 8))
static uint16_t width_curr = 0;
static uint16_t height_curr = 0;

typedef struct _lcd_ctl
{
    uint8_t mode;  //0 polling 1 interrupt
    uint8_t dir;
    uint16_t width;
    uint16_t height;
    uint16_t start_offset_w0;
    uint16_t start_offset_h0;
    uint16_t start_offset_w1;
    uint16_t start_offset_h1;
    uint16_t start_offset_w;
    uint16_t start_offset_h;
} mcu_lcd_ctl_t;


static uint16_t* g_lcd_display_buff = NULL;
static uint16_t g_lcd_w = 0;
static uint16_t g_lcd_h = 0;
static bool g_lcd_init = false;
static mcu_lcd_ctl_t lcd_ctl;

uint8_t lcd_dis_get_zhCN_dat(uint8_t *zhCN_char, uint8_t *zhCN_dat);

void lcd_set_direction(lcd_dir_t dir)
{
    if(!g_lcd_init)
        return;
    //dir |= 0x08;  //excahnge RGB
    lcd_ctl.dir = ((lcd_ctl.dir & DIR_RGB2BRG) == DIR_RGB2BRG) ? (dir | DIR_RGB2BRG) : dir;

    if (lcd_ctl.dir & DIR_XY_MASK)
    {
        lcd_ctl.width = g_lcd_w - 1;
        lcd_ctl.height = g_lcd_h - 1;
        lcd_ctl.start_offset_w = lcd_ctl.start_offset_w1;
        lcd_ctl.start_offset_h = lcd_ctl.start_offset_h1;
    }
    else
    {
        lcd_ctl.width = g_lcd_h - 1;
        lcd_ctl.height = g_lcd_w - 1;
        lcd_ctl.start_offset_w = lcd_ctl.start_offset_w0;
        lcd_ctl.start_offset_h = lcd_ctl.start_offset_h0;
    }
    
    tft_write_command(MEMORY_ACCESS_CTL);
    tft_write_byte((uint8_t *)&lcd_ctl.dir, 1);
}


void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {0};

    x1 += lcd_ctl.start_offset_w;
    x2 += lcd_ctl.start_offset_w;
    y1 += lcd_ctl.start_offset_h;
    y2 += lcd_ctl.start_offset_h;

    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2);
    tft_write_command(HORIZONTAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2);
    tft_write_command(VERTICAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    tft_write_command(MEMORY_WRITE);
}

int lcd_init(void)
{
    int ret = 0;
    // uint16_t color = MARINE_BLUE;
    uint16_t color = BLACK;//默认背景黑色
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM); //RST_PIN
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);//DCX_PIN
    fpioa_set_function(36, FUNC_SPI0_SS0 + LCD_SPI_SLAVE_SELECT);//CS_PIN
    fpioa_set_function(39, FUNC_SPI0_SCLK);//CLK_PIN

    //lcd init
    uint8_t data = 0;
    lcd_ctl.dir =DIR_YX_RLDU;//DIR_XY_RLUD;//竖屏  //DIR_YX_RLDU�?�?
    lcd_ctl.width = 320;
    lcd_ctl.height = 240;
    lcd_ctl.start_offset_w0 =0;
    lcd_ctl.start_offset_h0 = 0;
    lcd_ctl.start_offset_w1 = 0;
    lcd_ctl.start_offset_h1 = 0;
    lcd_ctl.start_offset_h = 0;
    lcd_ctl.start_offset_w = 0;
    // printf("w: %d, h: %d, freq: %d, invert: %d, %d, %d, %d, %d, %d\r\n", lcd_para->width, 
    // lcd_para->height, lcd_para->freq, lcd_para->invert, lcd_para->offset_w0, lcd_para->offset_w1,
    // lcd_para->offset_h1, lcd_para->offset_h0, lcd_para->oct);
    if(g_lcd_w != lcd_ctl.width || g_lcd_h !=  lcd_ctl.height)
    {
        if(g_lcd_display_buff)
        {
            free(g_lcd_display_buff);
        }
        g_lcd_display_buff = (uint16_t*)malloc(lcd_ctl.width* lcd_ctl.height*2);
        if(!g_lcd_display_buff)
            return 12; //ENOMEM
        g_lcd_w = lcd_ctl.width;
        g_lcd_h =  lcd_ctl.height;
    }
    tft_hard_init(15000000, 1);
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    msleep(50);

    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    msleep(120);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x55;
    tft_write_byte(&data, 1);
    msleep(10);
    
    g_lcd_init = true;

    lcd_set_direction(lcd_ctl.dir);
    // if(lcd_para->invert)
    // {
    //     tft_write_command(INVERSION_DISPALY_ON);
    //     msleep(10);
    // }
    tft_write_command(NORMAL_DISPALY_ON);
    msleep(10);
    /*display on*/
    tft_write_command(DISPALY_ON);
    // msleep(100);
    lcd_ctl.mode = 0; //polling mode

     if (0 != lcd_ctl.dir)
    {
        lcd_set_direction(lcd_ctl.dir);
    }
    //clear lcd
    lcd_clear(color);
     if (ret != 0)
    {
        lcd_ctl.width = 0;
        lcd_ctl.height  = 0;
        width_curr = 0;
        height_curr = 0;
        printf("lcd init error:%d\r\n",ret);
        return 11;
    }else{
        // lcd_ctl.width = lcd-get_width();
        // lcd_para.height = lcd->get_height();
        
        
        height_curr =  lcd_ctl.height;
        width_curr  = lcd_ctl.width;
        // height_curr =  lcd_ctl.width;
        // width_curr  = lcd_ctl.height;
    }
    return 0;
}


// static bool check_rotation(mp_int_t r)
// {
//     if (r > 3 || r < 0)
//         return false;
//     return true;
// }



static lcd_dir_t get_dir_by_rotation(uint8_t rotation)
{
    lcd_dir_t v = DIR_YX_RLDU;
    switch (rotation)
    {
    case 0:
        v = DIR_YX_RLDU;
        break;
    case 1:
        v = DIR_XY_RLUD;
        break;
    case 2:
        v = DIR_YX_LRUD;
        break;
    case 3:
        v = DIR_XY_LRDU;
        break;
    }
    return v;
}

void lcd_clear(uint16_t rgb565_color)
{
    #if LCD_SWAP_COLOR_BYTES
        rgb565_color = SWAP_16(rgb565_color);
    #endif
    uint32_t data = ((uint32_t)rgb565_color << 16) | (uint32_t)rgb565_color;
    lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
    tft_fill_data(&data, g_lcd_h * g_lcd_w / 2);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_area(x, y, x, y);
    tft_write_byte((uint8_t*)&color, 2);
}

/**
 * @brief draw char on the LCD screen
 *
 * @param x x coordinate of the start point in pixels.
 * @param y y coordinate of the start point in pixels.
 * @param c character to display.
 * @param color color index for the character.
 */
static void lcd_draw_char(uint16_t x, uint16_t y, uint8_t c, uint16_t color,uint16_t back_color)
{
    uint8_t i = 0;
    /* ������������ڴ洢�ַ������� */
    uint8_t j = 0;
    uint8_t data = 0;

    for (i = 0; i < 16; i++)
    /* �� ASCII ���ж�ȡ�ַ������ݣ�������洢�� data ������ */
    {
        data = ascii0816[c * 16 + i];
        /* ��ȡ ASCII ���е��ַ����� */
        for (j = 0; j < 8; j++)
        {
            if (data & 0x80)
            /* ������ݵ����λ�Ƿ�Ϊ 1�����������Ƶ� */
                lcd_draw_point(x + j, y, color);
            else
            {
                lcd_draw_point(x + j, y, back_color);
            }
            /* �����ݵ����λ����һ�� bit���Ա���������һ���ַ� */
            data <<= 1;
        }
        y++;
    }
}


static void lcd_draw_zh_char(uint8_t *zhCh_char,uint16_t x, uint16_t y,  uint16_t color)
{
    uint8_t zhCN_dat[128]; //max 16*16
    memset(zhCN_dat,0xff,sizeof(zhCN_dat));

    lcd_dis_get_zhCN_dat(zhCh_char,zhCN_dat);
    
    // printf("zh_char:%x %x:",*zhCh_char,*(zhCh_char+1));
    // for(int i=1;i<=2;i++)
    // {
    //     for(int j=0;j<16;j++)
    //         printf(" %x",zhCN_dat[i*j]);
    //     printf("\n");
    // }
    
     uint8_t data;
    uint16_t *addr = NULL;
    uint16_t oy = y;
    for(uint8_t i=0;i<32;i++)
    {
        data = *(zhCN_dat+i);
        for(uint8_t j=0;j<8;j++)
        {
            // addr = y*240+x;
            if(data & 0x80)
            {
                lcd_draw_point(x, y, color);
            }
            data <<= 1;
            y++;
            if((y-oy)== 16)
            {
                y=oy;
                x++;
                break;
            }
        }
    }
}
void lcd_draw_string(uint16_t x, uint16_t y, uint8_t *str, uint16_t color,uint16_t back_color)
{
#if LCD_SWAP_COLOR_BYTES
    color = SWAP_16(color);
#endif
     uint8_t have_zhCN = 0;
    uint16_t ox = x;
    uint16_t oy = y;
    uint8_t size = 16;
    while (*str)
    {
        //�ж��Ƿ�Ϊ���ģ�������������ȡgb2312λ�룬����������ֱ����ʾ
        if(have_zhCN == 0)
        {
            if(*str > 0x80)
            {
                have_zhCN = 1; // ���Ϊ���ڴ��������ַ�
            }
            else
            {
                // if (x > (ox + 240 - size / 2)) // �������ͼ�����
                // {
                //     y += size; // ����
                //     x = ox; // ����x����
                // }
                if (y > (oy + 320 - size)) // �������ͼ��߶�
                    break; // �˳�ѭ��

                if ((*str == 0xd) || // ����ǻس������з�
                    (*str == 0xa))
                {
                    y += size; // ����
                    x = ox; // ����x����
                    str++; // �ƶ�����һ���ַ�
                }
                else
                {
                    
                    lcd_draw_char(x, y, *str, color,back_color);//��ʾӢ���ַ�
                }
                str++;          // �ƶ�����һ���ַ�
                x+= 8;  // ����x����
            }
            
        }
        else
        {
            have_zhCN = 0; // ���ñ��
            // if (x > (ox + img_w - size)) // �������ͼ�����
            // {
            //     y += size; // ����
            //     x = ox; // ����x����
            // }
            if (y > (oy + 320 - size)) // �������ͼ��߶�
                break; // �˳�ѭ��
            
            lcd_draw_zh_char(str,x,y,color);
            str += 2; // �ƶ�����һ���ַ��������ַ�ռ�����ֽڣ�
            x += size; // ����x����
        }
        
        // str++;
        // x += 8;
    }
}
static void ram_draw_char(uint32_t *ptr, uint16_t x, uint16_t y, char c, uint16_t color)
{
    uint8_t i, j, data;
    uint16_t *addr;

    for(i = 0; i < 16; i++)
    {
        addr = ((uint16_t *)ptr) + y * (lcd_ctl.width + 1) + x;
        data = ascii0816[c * 16 + i];
        for(j = 0; j < 8; j++)
        {
            if(data & 0x80)
            {
                if((x + j) & 1)
                    *(addr - 1) = color;
                else
                    *(addr + 1) = color;
            }
            data <<= 1;
            addr++;
        }
        y++;
    }
}
static void ram_draw_zh_char(uint32_t *ptr,uint16_t x, uint16_t y, uint8_t  *zhCh_char, uint16_t color)
{
    uint8_t zhCN_dat[128]; //max 16*16
    memset(zhCN_dat,0xff,sizeof(zhCN_dat));

    lcd_dis_get_zhCN_dat(zhCh_char,zhCN_dat);
        printf("zh_char:%x %x:",*zhCh_char,*(zhCh_char+1));
    for(int i=1;i<=2;i++)
    {
        for(int j=0;j<16;j++)
            printf(" %x",zhCN_dat[i*j]);
        printf("\n");
    }
    uint8_t data;
    uint16_t *addr = NULL;
    uint16_t oy = y;
    for(uint8_t i=0;i<32;i++)
    {
        data = *(zhCN_dat+i);
        addr = ((uint16_t *)ptr) + y * (lcd_ctl.width + 1) + x;
        for(uint8_t j=0;j<8;j++)
        {
            if(data & 0x80)
            {
                if((x + j) & 1)
                    *(addr - 1) = color;
                else
                    *(addr + 1) = color;
            }
            data <<= 1;
            addr++;
            y++;
            if((y-oy)== 16)
            {
                y=oy;
                x++;
                break;
            }
        }
    }
}
void ram_draw_string(uint32_t *ptr, uint16_t x, uint16_t y, char *str, uint16_t color)
{
    uint8_t have_zhCN = 0;
    
    while(*str)
    {
        if(have_zhCN == 0)
        {
            if(*str > 0x80)
            {
                have_zhCN = 1;
            }
            else
            {
                ram_draw_char(ptr, x, y, *str, color);
                str++;
                x += 8;
            }
        }
        else
        {
            have_zhCN = 0; 
            ram_draw_zh_char(ptr,x,y,str,color);
            str+=2;
            x+= 16; 
        }

    }
}
static uint16_t* g_pixs_draw_pic = NULL;
static uint32_t g_pixs_draw_pic_size = 0;
static uint32_t g_pixs_draw_pic_half_size = 0;
void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr)
{
    uint32_t i;
    uint16_t* p = (uint16_t*)ptr;
    bool odd = false;
    // extern volatile bool maixpy_sdcard_loading;

    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
    tft_write_word(ptr,width*height/2);
    return ;
    g_pixs_draw_pic_size = width*height;
    // printk("\t%d %d %d %d, %d %d %d\r\n", x1, y1, x1 + width - 1, y1 + height - 1, width, height, g_pixs_draw_pic_size);
    /*
    for(i=0; i<g_pixs_draw_pic_size; ++i )
    {
        g_lcd_display_buff[i] = SWAP_16(*p);
        ++p;
    }
    tft_write_half(g_lcd_display_buff, g_pixs_draw_pic_size);
    */
    if(g_pixs_draw_pic_size % 2)
    {
        odd = true;
        g_pixs_draw_pic_size -= 1;
    }
    if( g_pixs_draw_pic_size > 0)
    {
        // if (maixpy_sdcard_loading) {
        //     for(i=0; i< g_pixs_draw_pic_size; i+=2)
        //     {
        //         #if LCD_SWAP_COLOR_BYTES
        //             g_lcd_display_buff[i] = SWAP_16(*(p+1));
        //             g_lcd_display_buff[i+1] = SWAP_16(*(p));
        //         #else
        //             g_lcd_display_buff[i] = *(p+1);
        //             g_lcd_display_buff[i+1] = *p;
        //         #endif
        //         p+=2;
        //     }
        // } else 
        {
            g_pixs_draw_pic_half_size = g_pixs_draw_pic_size/2;
            g_pixs_draw_pic_half_size = (g_pixs_draw_pic_half_size%2) ? (g_pixs_draw_pic_half_size+1) : g_pixs_draw_pic_half_size;
            g_pixs_draw_pic = p+g_pixs_draw_pic_half_size;
            // dual_func = swap_pixs_half;
            for(i=0; i< g_pixs_draw_pic_half_size; i+=2)
            {
                #if LCD_SWAP_COLOR_BYTES
                    g_lcd_display_buff[i] = SWAP_16(*(p+1));
                    g_lcd_display_buff[i+1] = SWAP_16(*(p));
                #else
                    g_lcd_display_buff[i] = *(p+1);
                    g_lcd_display_buff[i+1] = *p;
                #endif
                p+=2;
            }
            // while(dual_func){}
        }
        tft_write_word((uint32_t*)g_lcd_display_buff, g_pixs_draw_pic_size / 2);
    }
    if( odd )
    {
        #if LCD_SWAP_COLOR_BYTES
            g_lcd_display_buff[0] = SWAP_16( ((uint16_t*)ptr)[g_pixs_draw_pic_size]);
        #else
            g_lcd_display_buff[0] = ((uint16_t*)ptr)[g_pixs_draw_pic_size];
        #endif
        lcd_set_area(x1 + width - 1, y1 + height - 1, x1 + width - 1, y1 + height - 1);
        tft_write_half(g_lcd_display_buff, 1);
    }
}
void lcd_set_rotation(uint8_t rotate)
{
    
    if (rotate > 3 || rotate < 0)
        return ;
    switch (rotate)
    {
    case 0:
    case 2:
        width_curr = lcd_ctl.width;
        height_curr = lcd_ctl.height;
        break;
    case 1:
    case 3:
        width_curr = lcd_ctl.height;
        height_curr = lcd_ctl.width;
    break;
    default:
        break;
    }
    lcd_dir_t dir =DIR_YX_RLDU ;
    switch (rotate)
    {
    case 0:
        dir = DIR_YX_RLDU;
        break;
        case 1:
        dir = DIR_XY_RLUD;
        break;
    case 2:
        dir = DIR_YX_LRUD;
        break;
    case 3:
        dir = DIR_XY_LRDU;
        break;
    default:
        break;
    }
    lcd_set_direction(dir);
}
//GB2312 //ȡģ��ʽ�� ���룬����ʽ��˳��
uint8_t lcd_dis_get_zhCN_dat(uint8_t *zhCN_char, uint8_t *zhCN_dat)
{
    uint8_t ch, cl;
    uint32_t font_offset;

    // uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);  
    uint8_t csize = 32;

    ch = *zhCN_char;
    cl = *(++zhCN_char);

    if (ch < 0xa1 || cl < 0xa0 ||
        ch > 0xf7 ||
        (ch >= 0xaa && ch <= 0xaf))
    {
        return 0;
    }

    ch -= 0xa1;
    cl -= 0xa0;

    font_offset = (ch * 96 + cl) * csize;
    w25qxx_read_data((uint32_t)(FONT_DATA_ADDR + font_offset), zhCN_dat, csize, W25QXX_QUAD_FAST);
    
    return 1;
}