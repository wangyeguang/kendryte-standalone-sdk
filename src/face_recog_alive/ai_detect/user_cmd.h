/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-05-09 14:58:38
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-05-15 15:48:47
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\user_cmd.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _USER_CMD_H
#define _USER_CMD_H

#include "stdint.h"


// int user_cmd_port_init();
int user_cmd_init(void);
void print_recv(void);
int user_cmd_recvdata(uint8_t *buf,int len);
int user_cmd_senddata(uint8_t *buf,int len);
int user_cmd_process();
#endif