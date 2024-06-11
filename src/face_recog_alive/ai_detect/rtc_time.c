/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-06 14:38:24
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\rtc_time.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "rtc_time.h"
#include "sipeed_i2c.h"
#include "sleep.h"
#include "fpioa.h"
#include<stdio.h>
#include<stdlib.h>
#include <stdint.h>

// typedef unsigned char uint8_t;
#define RTC_ADDR 0x68
static int i2c_num = I2C_DEVICE_2;
#define dec2bcd(n) ((n/10*16)+(n%10))
#define bcd2dec(n) ((n/16*10)+(n%16))

/*默认使用k210 I2C2端口 scl = 14 sda = 15*/
void ex_rtc_init(void)
{
    fpioa_set_function(14, FUNC_I2C2_SCLK);//i2c_pin_clk
	fpioa_set_function(15, FUNC_I2C2_SDA);

    maix_i2c_init(i2c_num, 7, 100000);//i2c addr= 0
}
static int i2c_write_reg(uint8_t reg_addr,uint8_t reg_data)
{
    uint8_t tmp[3] = {0};
    tmp[0] = reg_addr & 0xFF;
    tmp[1] = reg_data;
    return maix_i2c_send_data(i2c_num,RTC_ADDR,tmp,2,10);
}
static int i2c_read_reg(uint8_t reg_addr)
{
    uint8_t data = 0;
    maix_i2c_send_data(i2c_num, RTC_ADDR, (uint8_t *)&reg_addr, 1, 10);
    maix_i2c_recv_data(i2c_num,RTC_ADDR,NULL,0,(uint8_t *)&data,1,10);
    if(data == 0xff)
        return -1;
    return data;
}
int ex_rtc_set_time(uint8_t day,uint8_t month,int year,uint8_t weekday, uint8_t hour, uint8_t min, uint8_t sec)
{
    uint8_t set_year = year-2000;
    set_year = dec2bcd(set_year);
    uint8_t set_month = dec2bcd(month);
    uint8_t set_weekday = dec2bcd(weekday);
    uint8_t set_day = dec2bcd(day);
    uint8_t set_minute = dec2bcd(min);
    uint8_t set_hour = hour & 0x1F;
    set_hour = dec2bcd(set_hour);
    uint8_t set_second = dec2bcd(sec);
    // printf("ex rtc set time:%x-%x-%x_%x:%x:%x\n",set_year,set_month,set_day,set_hour,set_minute,set_second);
    i2c_write_reg(0x06, set_year);
    i2c_write_reg(0x05, set_month);
    i2c_write_reg(0x04, set_day);
    i2c_write_reg(0x03, set_weekday);
    i2c_write_reg(0x02, set_hour);
    i2c_write_reg(0x01, set_minute);
    set_second = set_second & 0x80;
    i2c_write_reg(0x00, set_second);
    // msleep(1000);
    // int get_year = 0;
    // set_day = 0;
    // set_month = 0;
    // set_hour = 0;
    // set_weekday = 0;
    // set_year = 0;
    // set_minute = 0;
    // set_second = 0;
    // ex_rtc_get_time(&set_day,&set_month,&get_year,&set_weekday,&set_hour,&set_minute,&set_second);
    // if(set_day == 0 && set_month == 0)
    //     return 0;
    return 1;
}

int ex_rtc_get_time(uint8_t *day,uint8_t *month,int *year,uint8_t *weekday,uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    uint8_t temp = 0;
    temp = i2c_read_reg(0x06);
    *year = bcd2dec(temp)+2000;
    temp = i2c_read_reg(0x05);
    *month = bcd2dec(temp);
    temp = i2c_read_reg(0x04);
    *day = bcd2dec(temp);
    temp = i2c_read_reg(0x03);
    *weekday = bcd2dec(temp);
    temp = i2c_read_reg(0x02);
    *hour = bcd2dec(temp);
    temp = i2c_read_reg(0x01);
    *min = bcd2dec(temp);
    temp = i2c_read_reg(0x00);
    *sec = bcd2dec(temp);

    // printf("RTC Time: %d_%02d:%02d:%02d\n", *day,*hour, *min, *sec);
    return 1;
}

#define LEAPOCH ((31 + 29) * 86400)

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)

static const uint16_t days_since_jan1[]= { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

bool timeutils_is_leap_year(uint64_t year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

// month is one based
uint64_t timeutils_days_in_month(uint64_t year, uint64_t month) {
    uint64_t mdays = days_since_jan1[month] - days_since_jan1[month - 1];
    if (month == 2 && timeutils_is_leap_year(year)) {
        mdays++;
    }
    return mdays;
}

// compute the day of the year, between 1 and 366
// month should be between 1 and 12, date should start at 1
uint64_t timeutils_year_day(uint64_t year, uint64_t month, uint64_t date) {
    uint64_t yday = days_since_jan1[month - 1] + date;
    if (month >= 3 && timeutils_is_leap_year(year)) {
        yday += 1;
    }
    return yday;
}
// returns the number of seconds, as an integer, since 2000-01-01
uint64_t timeutils_seconds_since_2000(uint64_t year, uint64_t month,
    uint64_t date, uint64_t hour, uint64_t minute, uint64_t second) {
    return
        second
        + minute * 60
        + hour * 3600
        + (timeutils_year_day(year, month, date) - 1
            + ((year - 2000 + 3) / 4) // add a day each 4 years starting with 2001
            - ((year - 2000 + 99) / 100) // subtract a day each 100 years starting with 2001
            + ((year - 2000 + 399) / 400) // add a day each 400 years starting with 2001
            ) * 86400
        + (year - 2000) * 31536000;
}
// returns the number of seconds, as an integer, since 1970-01-01
uint64_t timeutils_seconds_since_1970(uint64_t year, uint64_t month,
    uint64_t date, uint64_t hour, uint64_t minute, uint64_t second) {
    return
        second
        + minute * 60
        + hour * 3600
        + (timeutils_year_day(year, month, date) - 1 -1
            + ((year - 1970 + 3) / 4) 
            - ((year - 1970 + 99) / 100) 
            + ((year - 1970 + 399) / 400) 
            ) * 86400
        + (year - 1970) * 31536000;
}
static uint32_t mktime(int year,uint32_t month,uint32_t mday,uint32_t hours,uint32_t minutes,uint32_t seconds)
{
        printf("mktime start:year:%d month:%d mday:%d hours:%d minutes:%d seconds:%d\n",year,month,mday,hours,minutes,seconds);

    minutes += seconds / 60;
    if ((seconds = seconds % 60) < 0) {
        seconds += 60;
        minutes--;
    }

    hours += minutes / 60;
    if ((minutes = minutes % 60) < 0) {
        minutes += 60;
        hours--;
    }

    mday += hours / 24;
    if ((hours = hours % 24) < 0) {
        hours += 24;
        mday--;
    }
    month--; // make month zero based
    year += month / 12;
    if ((month = month % 12) < 0) {
        month += 12;
        year--;
    }
    month++; // back to one based

    while (mday < 1) {
        if (--month == 0) {
            month = 12;
            year--;
        }
        mday += timeutils_days_in_month(year, month);
    }
    while ((uint64_t)mday > timeutils_days_in_month(year, month)) {
        mday -= timeutils_days_in_month(year, month);
        if (++month == 13) {
            month = 1;
            year++;
        }
    }
    printf("mktime end:year:%d month:%d mday:%d hours:%d minutes:%d seconds:%d\n",year,month,mday,hours,minutes,seconds);
    return timeutils_seconds_since_1970(year, month, mday, hours, minutes, seconds);
}

uint32_t getTimestamp()
{
    int beijing_offset = 28800;
    uint8_t get_hour,get_min,get_sec,get_month,get_day,get_weekday;
    uint32_t get_year = 0;
    ex_rtc_get_time(&get_day,&get_month,&get_year,&get_weekday,&get_hour,&get_min,&get_sec);
    uint32_t current_time = mktime(get_year,get_month,get_day,get_hour,get_min,get_sec)-beijing_offset;
    return current_time;
}