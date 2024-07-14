/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-05-14 11:30:37
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-07-11 14:32:09
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\core1.h
 * @Description:  
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _CORE1_H
#define _CORE1_H

#include "cQueue.h"

/* clang-format off */
typedef enum _net_task_type
{
    TASK_UPLOAD                 = (0),

    TASK_CAL_PIC_FEA            = (2),
    TASK_CAL_PIC_FEA_RES        = (3),

} net_task_type_t;

/* clang-format on */

typedef struct _net_task
{
    net_task_type_t task_type;
    void *task_data;
} net_task_t;

/* clang-format on */
extern volatile Queue_t q_core1;

int core1_function(void *ctx);
#endif // !_CORE1_H