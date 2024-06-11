/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-05-11 09:57:24
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\adc_function.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _ADC_FUNCTION_H
#define _ADC_FUNCTION_H

void adc_init(void);
void ads1115_vcc_get_voltage_val();
float get_ACC_voltage_val();
float get_SEAT_voltage_val();

#endif