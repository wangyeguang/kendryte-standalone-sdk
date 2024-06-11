#ifndef _RTC_TIME_H
#define _RTC_TIME_H

#include <stdint.h>

void ex_rtc_init(void);
int ex_rtc_set_time(uint8_t day,uint8_t month,int year,uint8_t weekday, uint8_t hour, uint8_t min, uint8_t sec);
int ex_rtc_get_time(uint8_t *day,uint8_t *month,int *year,uint8_t *weekday,uint8_t *hour, uint8_t *min, uint8_t *sec);
uint32_t getTimestamp();
#endif // !_RTC_TIME_H
