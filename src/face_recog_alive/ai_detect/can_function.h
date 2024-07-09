#ifndef _CAN_FUNCTION_H
#define _CAN_FUNCTION_H

int can_init();
int can_set_filter(int *data,int len);
int can_set_baud(int baud);
int can_set_canType(int type); //设置CAN类型

#endif