#include "adc_function.h"
#include "sipeed_i2c.h"
#include "sleep.h"
#include "fpioa.h"
#include<stdio.h>
#include<stdlib.h>
#include <stdint.h>

// I2C地址
#define ADS1015_ADDRESS 0x48
// 转换延迟
#define ADS1015_CONVERSIONDELAY 1
#define ADS1115_CONVERSIONDELAY 15

// 定义ADS1015寄存器地址
#define ADS1015_REG_POINTER_MASK 0x03
#define ADS1015_REG_POINTER_CONVERT 0x00
#define ADS1015_REG_POINTER_CONFIG 0x01
#define ADS1015_REG_POINTER_LOWTHRESH 0x02
#define ADS1015_REG_POINTER_HITHRESH 0x03

// 定义配置寄存器
#define ADS1015_REG_CONFIG_OS_MASK 0x8000
#define ADS1015_REG_CONFIG_OS_SINGLE 0x8000       // 写入：设置为开始单个转换
#define ADS1015_REG_CONFIG_OS_BUSY 0x0000         // 读取：正在进行转换时位=0
#define ADS1015_REG_CONFIG_OS_NOTBUSY 0x8000      // 读取：当设备未执行转换时，位=1

#define ADS1015_REG_CONFIG_MUX_MASK 0x7000        //
#define ADS1015_REG_CONFIG_MUX_DIFF_0_1 0x0000    // 差分 P=AIN0，N=AIN1（默认值）
#define ADS1015_REG_CONFIG_MUX_DIFF_0_3 0x1000    // 差分 P=AIN0，N=AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_1_3 0x2000    // 差分 P=AIN1，N=AIN3;(差分输入1引脚和3引脚)
#define ADS1015_REG_CONFIG_MUX_DIFF_2_3 0x3000    // 差分 P=AIN2，N=AIN3
#define ADS1015_REG_CONFIG_MUX_SINGLE_0 0x4000    // 单端AIN0 (单端输入 0)
#define ADS1015_REG_CONFIG_MUX_SINGLE_1 0x5000    // 单端AIN1
#define ADS1015_REG_CONFIG_MUX_SINGLE_2 0x6000    // 单端AIN2
#define ADS1015_REG_CONFIG_MUX_SINGLE_3 0x7000    // 单端AIN3

#define ADS1015_REG_CONFIG_PGA_MASK 0x0E00        //
#define ADS1015_REG_CONFIG_PGA_6_144V 0x0000      // +/-6.144V范围=增益2/3
#define ADS1015_REG_CONFIG_PGA_4_096V 0x0200      // +/-4.096V范围=增益1
#define ADS1015_REG_CONFIG_PGA_2_048V 0x0400      // +/-2.048V范围=增益2（默认值）
#define ADS1015_REG_CONFIG_PGA_1_024V 0x0600      // +/-1.024V范围=增益4
#define ADS1015_REG_CONFIG_PGA_0_512V 0x0800      // +/-0.512V范围=增益8
#define ADS1015_REG_CONFIG_PGA_0_256V 0x0A00      // +/-0.256V范围=增益16

#define ADS1015_REG_CONFIG_MODE_MASK 0x0100
#define ADS1015_REG_CONFIG_MODE_CONTIN 0x0000     // 连续转换模式
#define ADS1015_REG_CONFIG_MODE_SINGLE 0x0100     // 断电单次触发模式（默认）

#define ADS1015_REG_CONFIG_DR_MASK 0x00E0         //
#define ADS1015_REG_CONFIG_DR_128SPS 0x0000       // 每秒128个样本
#define ADS1015_REG_CONFIG_DR_250SPS 0x0020       // 每秒250个样本
#define ADS1015_REG_CONFIG_DR_490SPS 0x0040       // 每秒490个样本
#define ADS1015_REG_CONFIG_DR_920SPS 0x0060       // 每秒920个样本
#define ADS1015_REG_CONFIG_DR_1600SPS 0x0080      // 每秒1600个样本（默认值）
#define ADS1015_REG_CONFIG_DR_2400SPS 0x00A0      // 每秒2400个样本
#define ADS1015_REG_CONFIG_DR_3300SPS 0x00C0      // 每秒3300个样本

#define ADS1015_REG_CONFIG_CMODE_MASK 0x0010
#define ADS1015_REG_CONFIG_CMODE_TRAD 0x0000      // 带滞后的传统比较器（默认）
#define ADS1015_REG_CONFIG_CMODE_WINDOW 0x0010    // 窗口比较器

#define ADS1015_REG_CONFIG_CPOL_MASK 0x0008
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW 0x0000    // 激活时ALERT/RDY引脚为低电平（默认值）
#define ADS1015_REG_CONFIG_CPOL_ACTVHI 0x0008     // 激活时ALERT/RDY引脚为高电平

#define ADS1015_REG_CONFIG_CLAT_MASK 0x0004       // 确定ALERT/RDY引脚在断言后是否锁存
#define ADS1015_REG_CONFIG_CLAT_NONLAT 0x0000     // 非锁存比较器（默认）
#define ADS1015_REG_CONFIG_CLAT_LATCH 0x0004      // 闭锁比较器

#define ADS1015_REG_CONFIG_CQUE_MASK 0x0003       //
#define ADS1015_REG_CONFIG_CQUE_1CONV 0x0000      // Assert ALERT/RDY after one conversions  在一次转换后断言ALERT/RDY
#define ADS1015_REG_CONFIG_CQUE_2CONV 0x0001      // Assert ALERT/RDY after two conversions  两次转换后断言ALERT/RDY
#define ADS1015_REG_CONFIG_CQUE_4CONV 0x0002      // Assert ALERT/RDY after four conversions 四次转换后断言ALERT/RDY
#define ADS1015_REG_CONFIG_CQUE_NONE 0x0003       // Disable the comparator and put ALERT/RDY in high state (default) 禁用比较器并将ALERT/RDY置于高状态（默认）

static int i2c_num = I2C_DEVICE_1;

static int m_gain = ADS1015_REG_CONFIG_PGA_6_144V;
static int m_conversionDelay = ADS1115_CONVERSIONDELAY;
static int m_bitShift = 0;

void adc_init(void)
{
    fpioa_set_function(33,FUNC_I2C1_SCLK); //
    fpioa_set_function(34,FUNC_I2C1_SDA);

    maix_i2c_init(i2c_num,7,400000);
}

static int i2c_write_reg(uint8_t reg_addr,uint16_t reg_data)
{
    uint8_t tmp[3] = {0};
    tmp[0] = reg_addr & 0xFF;
    tmp[1] = reg_data >> 8;
    tmp[2] = reg_data&0xFF;
    return maix_i2c_send_data(i2c_num,ADS1015_ADDRESS,tmp,3,10);
}

static int i2c_read_reg(uint8_t reg_addr)
{
    uint8_t data[2] = {0};
    maix_i2c_recv_data(i2c_num,ADS1015_ADDRESS, (uint8_t *)&reg_addr,1,(uint8_t *)&data,2,10);
    return (data[0] <<8 | data[1]);
    // if(data == 0xff)
        // return -1;
    // return data;
}

void setGain(int gain)
{
    m_gain = gain;
}
int getGain(void)
{
    return m_gain;
}
/* 获取指定通道的单端ADC读数 */
int readADC_SingleEnded(int channel) {
    if (channel > 3) {
        return 0;
    }

    int config = (
        ADS1015_REG_CONFIG_CQUE_NONE |
        ADS1015_REG_CONFIG_CLAT_NONLAT |
        ADS1015_REG_CONFIG_CPOL_ACTVLOW |
        ADS1015_REG_CONFIG_CMODE_TRAD |
        ADS1015_REG_CONFIG_DR_1600SPS |
        ADS1015_REG_CONFIG_MODE_SINGLE
    );
    config |= m_gain;

    if (channel == 0) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
    } else if (channel == 1) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
    } else if (channel == 2) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
    } else if (channel == 3) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
    }

    config |= ADS1015_REG_CONFIG_OS_SINGLE;
    i2c_write_reg(ADS1015_REG_POINTER_CONFIG, config);
    msleep(m_conversionDelay);
    return i2c_read_reg(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
}

// 读取ADC differential mode 0-1的函数
uint16_t readADC_Differential_0_1(void) {
    uint16_t config = 0;

    // 配置字段按位或运算
    config |= ADS1015_REG_CONFIG_CQUE_NONE;
    config |= ADS1015_REG_CONFIG_CLAT_NONLAT;
    config |= ADS1015_REG_CONFIG_CPOL_ACTVLOW;
    config |= ADS1015_REG_CONFIG_CMODE_TRAD;
    config |= ADS1015_REG_CONFIG_DR_1600SPS;
    config |= ADS1015_REG_CONFIG_MODE_SINGLE;
    config |= ADS1015_REG_CONFIG_MUX_DIFF_0_1;
    config |= m_gain;

    // 写入配置寄存器
    i2c_write_reg(ADS1015_REG_POINTER_CONFIG, config);

    // 等待转换完成（MicroPython特定的sleep函数被替换为标准的sleep）
    msleep(m_conversionDelay);

    // 读取转换结果
    uint16_t result = i2c_read_reg(ADS1015_REG_POINTER_CONVERT);

    return result >> m_bitShift; // 假设bitShift已经在前面被定义和初始化
}
// 读取ADC differential mode 2-3的函数
uint16_t readADC_Differential_2_3(void) {
    uint16_t config = 0;

    // 配置字段按位或运算
    config |= ADS1015_REG_CONFIG_CQUE_NONE;
    config |= ADS1015_REG_CONFIG_CLAT_NONLAT;
    config |= ADS1015_REG_CONFIG_CPOL_ACTVLOW;
    config |= ADS1015_REG_CONFIG_CMODE_TRAD;
    config |= ADS1015_REG_CONFIG_DR_1600SPS;
    config |= ADS1015_REG_CONFIG_MODE_SINGLE;
    config |= ADS1015_REG_CONFIG_MUX_DIFF_2_3;

    // 写入配置寄存器
    i2c_write_reg(ADS1015_REG_POINTER_CONFIG, config);

    // 等待转换完成（MicroPython特定的sleep函数被替换为标准的sleep）
    msleep(m_conversionDelay);

    // 读取转换结果
    uint16_t result = i2c_read_reg( ADS1015_REG_POINTER_CONVERT);

    return result >> m_bitShift; // 假设bitShift已经在前面被定义和初始化
}

void startComparator_SingleEnded(int channel, int threshold) {
    // 开始使用默认值
    int config = (
        ADS1015_REG_CONFIG_CQUE_1CONV |  // 比较器启用并在1个匹配时断言
        ADS1015_REG_CONFIG_CLAT_LATCH |  // 闩锁模式
        ADS1015_REG_CONFIG_CPOL_ACTVLOW |  // 警报/Rdy激活低（默认值）
        ADS1015_REG_CONFIG_CMODE_TRAD |  // 传统比较器（默认值）
        ADS1015_REG_CONFIG_DR_1600SPS |  // 每秒1600个样本（默认值）
        ADS1015_REG_CONFIG_MODE_CONTIN |  // 连续转换模式
        ADS1015_REG_CONFIG_MODE_CONTIN  // 连续转换模式
    );

    // 设置PGA/电压范围
    config |= m_gain;

    // 设置单端输入通道
    if (channel == 0) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
    } else if (channel == 1) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
    } else if (channel == 2) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
    } else if (channel == 3) {
        config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
    }

    // 设置高阈值寄存器
    // 将12位结果向左移动4位对于ADS1015
    i2c_write_reg(ADS1015_REG_POINTER_HITHRESH, threshold << m_bitShift);

    // 将配置寄存器写入ADC
    i2c_write_reg(ADS1015_REG_POINTER_CONFIG, config);
}

int getLastConversionResults(void) {
    // Wait for the conversion to complete
    msleep(m_conversionDelay);

    // Read the conversion results
    int res = i2c_read_reg(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
    if (m_bitShift == 0) {
        return res;
    } else {
        // Shift 12-bit results right 4 bits for the ADS1015,
        // making sure we keep the sign bit intact
        if (res > 0x07FF) {
            // negative number - extend the sign to 16th bit
            res |= 0xF000;
        }
        return res;
    }
}

void ads1115_vcc_get_voltage_val(float *acc,float *seat) {
    int setGainval = ADS1015_REG_CONFIG_PGA_6_144V;
    float multiplier;
    int adc0 = readADC_SingleEnded(0);
    int adc1 = readADC_SingleEnded(1);
    startComparator_SingleEnded(0, 1000);
    int adc00 = getLastConversionResults();
    setGain(setGainval);
    int gain = 0;
    gain = getGain();

    if (setGainval == ADS1015_REG_CONFIG_PGA_6_144V) {
        multiplier = 0.1875;
    } else if (setGainval == ADS1015_REG_CONFIG_PGA_4_096V) {
        multiplier = 0.125;
    } else if (setGainval == ADS1015_REG_CONFIG_PGA_2_048V) {
        multiplier = 0.0625;
    } else if (setGainval == ADS1015_REG_CONFIG_PGA_1_024V) {
        multiplier = 0.03125;
    } else if (setGainval == ADS1015_REG_CONFIG_PGA_0_512V) {
        multiplier = 0.015625;
    } else if (setGainval == ADS1015_REG_CONFIG_PGA_0_256V) {
        multiplier = 0.0078125;
    }

    *acc = (adc0 / 365) * 11365 * multiplier / 1000;
    *seat = (adc1 / 365) * 11365 * multiplier / 1000;

    if (*acc > 380) {
        *acc = 0;
    }
    if (*seat > 380) {
        *seat = 0;
    }
}

float get_ACC_voltage_val() {
    float acc_sum = 0;
    float acc = 0;
    for (size_t i = 0; i < 5; i++)
    {
        startComparator_SingleEnded(0, 1000);
        msleep(5);
        int adc00 = getLastConversionResults();
        acc = adc00*0.005838;
        if (acc > 380)  acc = 0;
        acc_sum += acc;
    }
    
    
    return acc;
}

float get_SEAT_voltage_val() {
    float seat_sum = 0;
    float seat = 0;
    for(int i=0;i<5;i++)
    {
        startComparator_SingleEnded(1, 1000);
        msleep(5);
        int adc00 = getLastConversionResults();
        seat = adc00*0.005838;
        if (seat > 380) seat = 0;
        seat_sum += seat;
    }
    seat = seat_sum/5;
    

    return seat;
}