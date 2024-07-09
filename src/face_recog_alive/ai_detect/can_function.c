/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-05-09 15:31:29
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-09 18:10:45
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\can_function.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "can_function.h"
#include "spi.h"
#include "gpio.h"
#include "gpiohs.h"
#include "fpioa.h"
#include "sleep.h"

    int can_sck = 10;//    #new
    int can_si  = 11;           //MISO
    int can_so  = 12;           //MOSI
    int can_cs  = 13;

int can_init() {
    // Initialize CAN peripheral and related resources
    // Configure CAN baud rate, filters, etc.
    // Return 0 on success, non-zero on error

    return 0;
}
#define mp_spi_pin_output(pin) gpiohs_set_drive_mode(pin, GPIO_DM_OUTPUT)

#define mp_spi_pin_input(pin) gpiohs_set_drive_mode(pin, GPIO_DM_INPUT)

static void mp_spi_pin_write(uint8_t pin, uint8_t val)
{
    gpiohs_set_pin(pin, val);
    mp_spi_pin_output(pin);
    mp_spi_pin_input(pin);
}
static int mp_spi_pin_read(uint8_t pin)
{
    mp_spi_pin_input(pin);
    return gpiohs_get_pin(pin);
}
static void mp_soft_spi_transfer(int len, const uint8_t *src,uint8_t *dest)
{
    for(int i=0;i<len;i++)
    {
        uint8_t data_out = src[i];
        uint8_t data_in = 0;
        for (int j = 0; j < 8; ++j, data_out <<= 1) {
            mp_spi_pin_write(FUNC_GPIOHS0+can_si, (data_out >> 7) & 1);
            mp_spi_pin_write(can_sck, 1);
            // if (self->pin_d[1] != -1)
            {
                data_in = (data_in << 1) | mp_spi_pin_read(FUNC_GPIOHS0+can_so);
            }
            mp_spi_pin_write(can_sck, 0);
        }
            if (dest != NULL) {
                dest[i] = data_in;
            }
    }
}
static void mp_soft_spi_write(uint8_t data)
{
    mp_soft_spi_transfer(1,(const uint8_t *)&data,NULL);
}
static void mp_soft_spi_write_readinfo(int len,uint8_t *write_buf,uint8_t *read_buf)
{
    mp_soft_spi_transfer(len,(const uint8_t *)write_buf,read_buf);
}
// static void mp_soft_spi_readinfo(int len,uint8_t write_byte,uint8_t *read_buf)
// {
//     mp_soft_spi_transfer(len,(const uint8_t *)write_buf,read_buf);
// }

/**
 * @brief  读一个字节
 * @note   
 * @param  can_addr: 
 * @retval 
 */
uint8_t MCP2515_ReadByte(uint8_t can_addr)
{
    uint8_t rbyte = 0;
    uint8_t write_byte = 0xff;
    gpiohs_set_pin(37,GPIO_PV_LOW);//cs 0
    mp_soft_spi_write(0x03);
    mp_soft_spi_write(can_addr);
    mp_soft_spi_transfer(1,(const uint8_t *)&write_byte,&rbyte);
    gpiohs_set_pin(37,GPIO_PV_HIGH);//cs 1
    return rbyte;
}
/**
 * @brief  写一个字节到MCP2515指定地址寄存器
 * @note   
 * @param  can_addr: 
 * @param  can_data: 
 * @retval None
 */
void MCP2515_WriteByte(uint8_t can_addr,uint8_t can_data)
{
    gpiohs_set_pin(37,GPIO_PV_LOW);//cs 0
    mp_soft_spi_write(0x02);
    mp_soft_spi_write(can_addr);
    mp_soft_spi_write(can_data);
    gpiohs_set_pin(37,GPIO_PV_HIGH);//cs 1
}

void MPC2515_Send_Buffer(int data_len,uint8_t *data)
{
    for (int i = 0; i < data_len; i++)
    {
        int dly = 0;
        while((MCP2515_ReadByte(0x30) &0x08)  && (dly<50))
        {
            dly++;
            msleep(1);
        }
        int j=0;
        for(j=0;j<data_len;j++)
        {
            MCP2515_WriteByte(0x36+j,data[i]);
        }
        MCP2515_WriteByte(0x35,j);
        MCP2515_WriteByte(0x30,0x08);
    }
}
void MCP2515_EnterConfigMode(void)
{
    MCP2515_WriteByte(0x0F,0x84);
}
void MCP2515_EnterNormalMode(void)
{
    MCP2515_WriteByte(0x0F,0x84);
}
void MCP2515_SetFilter(int list_len,uint8_t *filter_list)
{
    uint8_t sid_high = 0;
    uint8_t sid_low = 0;
    uint8_t mask_high = 0xff;
    uint8_t mask_low = 0xe0;
    for(int i=0;i<list_len;i++)
    {
        sid_high = sid_high | ((filter_list[i]>>3) & 0xff);
        sid_low = sid_low | ((filter_list[i]<<5) & 0xe0);
        mask_high = mask_high ^ ((filter_list[i]>>3) & 0xff);
        mask_low = mask_low ^ ((filter_list[i]<<5) & 0xe0);
    }
    MCP2515_WriteByte(0x00,sid_high);
    MCP2515_WriteByte(0x01,sid_low);
    MCP2515_WriteByte(0x20,mask_high);//配置验收屏蔽寄存器n标准标识符高位
    MCP2515_WriteByte(0x21,mask_low);//配置验收屏蔽寄存器n标准标识符低位
}
/**
 * @brief  
 * @note   
 * @retval 
 */
static int MCP2515_Reset(void)
{
    gpiohs_set_pin(37,GPIO_PV_LOW);
    mp_soft_spi_write(0xC0);//0xC0 发送寄存器复位命令
    gpiohs_set_pin(37,GPIO_PV_HIGH);
}

/**
 * @brief  phase=0 baudrate=10000000, polarity=0,bits=8, firstbit=SPI.MSB
 * @note   
 * @retval 
 */
int MCP2515_init(int baudrate)
{
    fpioa_set_function(can_cs,FUNC_GPIOHS0+can_cs);//24+13=37
    fpioa_set_function(can_si,FUNC_GPIOHS0+can_si); //MOSI output
    fpioa_set_function(can_so,FUNC_GPIOHS0+can_so); //MISO input
    gpiohs_set_drive_mode(37,GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(FUNC_GPIOHS0+can_si,GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(FUNC_GPIOHS0+can_so,GPIO_DM_INPUT);
    fpioa_set_function(can_sck, FUNC_GPIOHS0+can_sck);
    mp_spi_pin_write(can_sck,0);//polarity = 0
    mp_spi_pin_output(can_sck);

    MCP2515_Reset();
    msleep(1);
    MCP2515_WriteByte(0x2A, 0x00);
    if (baudrate == 500000)
            MCP2515_WriteByte(0x2A,0x00);
    else if(baudrate == 250000)
            MCP2515_WriteByte(0x2A,0x01);
    else if( baudrate == 125000)
            MCP2515_WriteByte(0x2A,0x03);
    else if( baudrate == 100000)
            MCP2515_WriteByte(0x2A,0x04);
    else if( baudrate == 50000)
            MCP2515_WriteByte(0x2A,0x09);
    else if( baudrate == 25000)
            MCP2515_WriteByte(0x2A,0x13);
    else if( baudrate == 10000)
            MCP2515_WriteByte(0x2A,0x31);

    // CNF2: 设置SAM=0, PHSEG1=(2+1)TQ=3TQ, PRSEG=(0+1)TQ=1TQ
    MCP2515_WriteByte(0x29, 0x90);

    // CNF3: PHSEG2=(2+1)TQ=3TQ, 同时在CANCTRL.CLKEN=1时设定CLKOUT引脚为时间输出使能位
    MCP2515_WriteByte(0x28, 0x02);

    // 发送缓冲器设置
    MCP2515_WriteByte(0x31, 0xFF); // 标准标识符高位
    MCP2515_WriteByte(0x32, 0xEB); // 标准标识符低位
    MCP2515_WriteByte(0x33, 0xFF); // 扩展标识符高位
    MCP2515_WriteByte(0x34, 0xFF); // 扩展标识符低位

    // 接收缓冲器设置
    MCP2515_WriteByte(0x61, 0x00); // 标准标识符高位
    MCP2515_WriteByte(0x62, 0x00); // 标准标识符低位
    MCP2515_WriteByte(0x63, 0x00); // 扩展标识符高位
    MCP2515_WriteByte(0x64, 0x00); // 扩展标识符低位
    MCP2515_WriteByte(0x60, 0x00); // 有效信息
    MCP2515_WriteByte(0x65, 0x08); // 设置接收数据长度为8字节

    // 设置滤波
    MCP2515_WriteByte(0x00, 0x44); // 配置验收滤波寄存器标准标识符高位
    MCP2515_WriteByte(0x01, 0x40); // 配置验收滤波寄存器标准标识符低位
    MCP2515_WriteByte(0x02, 0xFF);
    MCP2515_WriteByte(0x03, 0xFF);

    // 禁用未使用的过滤器
    MCP2515_WriteByte(0x08, 0x00);  // RXF2SIDH
    MCP2515_WriteByte(0x09, 0x00);  // RXF2SIDL
    MCP2515_WriteByte(0x0C, 0x00);  // RXF3SIDH
    MCP2515_WriteByte(0x0D, 0x00);  // RXF3SIDL
    MCP2515_WriteByte(0x10, 0x00);  // RXF4SIDH
    MCP2515_WriteByte(0x11, 0x00);  // RXF4SIDL
    MCP2515_WriteByte(0x14, 0x00);  // RXF5SIDH
    MCP2515_WriteByte(0x15, 0x00);  // RXF5SIDL

    // 设置屏蔽
    MCP2515_WriteByte(0x20, 0xFF); // 配置验收屏蔽寄存器标准标识符高位
    MCP2515_WriteByte(0x21, 0xE0); // 配置验收屏蔽寄存器标准标识符低位
    MCP2515_WriteByte(0x22, 0x00);
    MCP2515_WriteByte(0x23, 0x00);

    // 清空CAN中断标志寄存器
    MCP2515_WriteByte(0x2C, 0x00);

    // 配置CAN中断使能寄存器
    MCP2515_WriteByte(0x2B, 0x01);

    // 将MCP2515设置为正常模式, 退出配置模式
    MCP2515_WriteByte(0x0F, 0x04);

    // 读取CAN状态寄存器
    uint8_t temp = MCP2515_ReadByte(0x0E);
    printf("debug MCP2515_Init temp = %d\n", temp);

    if (0x00 != (temp & 0xE0)) {
        // 重新设置为正常模式, 退出配置模式
        MCP2515_WriteByte(0x0F, 0x04);
    }
}
