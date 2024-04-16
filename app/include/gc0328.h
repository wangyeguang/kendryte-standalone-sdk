/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-01 01:52:41
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-04-08 19:12:59
 * @FilePath: \kendryte-standalone-sdk\app\include\gc0328.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __GC0328_H
#define __GC0328_H

#include <stdint.h>

#define GC0328_ID       (0x9d)
#define GC0328_ADDR     (0x21)

int gc0328_init();
int gc0328_reset(void);
void open_gc0328_0(void);
void open_gc0328_1(void);
#endif /* __GC0328_H */
