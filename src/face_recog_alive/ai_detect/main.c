/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-03-29 19:38:04
 * @LastEditors: yeguang wang wangyeguang521@163.com
 * @LastEditTime: 2024-04-23 10:16:20
 * @FilePath: \kendryte-standalone-sdk\src\face_recog_alive\ai_detect\main.c
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
 * freertos版本
 * * 主线程实现人脸识别功能
 * * 看门狗线程维护系统运行
 * * CAN线程持续输出CAN数据
 */
#include <bsp.h>
#include <sysctl.h>
#include <stdio.h>
#include <fpioa.h>
#include <gpiohs.h>
#include <uarths.h>
#include <dmac.h>
#include <plic.h>
#include <rtc.h>
#include <dvp.h>
#include <gpio.h>
#include "device_config.h"
// #include <syslog.h>
#include "w25qxx.h"
// #include "flash.h"
#include "face_recognition.h"
#include "lcd.h"
#include "image.h"
// #include "sensor.h"
#include "gc0328.h"
#include "image_process.h"
#include "region_layer.h"
#include "esp8285.h"
#include "ai.h"


#define SOFTWARE_VERSION "1.0.0"
#define WIFI_SSID "beitababa"
#define WIFI_PASSWORD "12345678"
// volatile uint8_t g_ram_mux;
/*******fixed parameters*******/
#define FEATURE_LEN 192
/*******flexible parameters*******/
#define FACE_REGISTER_MODE FACE_APPEND
#define MAX_REGISTERED_FACE_NUMBER 100
#define FACE_RECGONITION_SCORE (80.5)

static image_t display_image;
static image_t kpu_image, resized_image;

kpu_model_context_t face_detect_task;
static region_layer_t face_detect_rl;
static key_point_t key_point;
static obj_info_t obj_detect_info;
static float registered_face_features[MAX_REGISTERED_FACE_NUMBER * FEATURE_LEN] = {0.0};
char display_status[32];
//显示输出图像
volatile static uint8_t g_dvp_finish_flag = 0;
uint8_t g_camera_no = 0;

volatile static uint8_t g_device_init_flag = 0;

// License 1>  0xdc9ef72c70f1d0ee 0x824c4a97ea681a6d
// License 2> 0xcebf3fb8739b629f 0x3d0ade17f30f383d

// License 1>  0x5b04cea6db6e268b 0x53e73272d762ad54
// License 2> 0x641ca3b44bbfb537 0xc581d1cca2d50fd9
license_t license[3] =
{ 
  {.lic1_h = 0xdc9ef72c70f1d0ee ,
     .lic1_l = 0x824c4a97ea681a6d,
     .lic2_h = 0xcebf3fb8739b629f ,
     .lic2_l = 0x3d0ade17f30f383d},
    {.lic1_h = 0x5b04cea6db6e268b,
    .lic1_l = 0x53e73272d762ad54,
    .lic2_h = 0x641ca3b44bbfb537,
    .lic2_l = 0xc581d1cca2d50fd9},
   
    {.lic1_h = 0x4b2e47f06872b8fd ,
     .lic1_l = 0xe7fceec9f3bd8baa,
     .lic2_h = 0xc81bb5c8cad98558 ,
     .lic2_l = 0x3102a276215f0c5f}
};
// static void ai_done(void *ctx)
// {
//     g_ai_done_flag = 1;
// }
static int dvp_irq(void *ctx)
{
      if(dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
        // LOGD("dvp_irq\n");
    } else
    {
      //  LOGD("dvp_start_convert\n");
        dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }
    return 0;
}

// static void	irled(uint32_t onoff)
// {
// 	int enable = onoff ? 1 : 0;
// 	// pwm_set_enable(1, 1, enable);

// }
void sys_reset(void)
{
	sysctl->soft_reset.soft_reset = 1;
}

void led_w_on()
{
  gpio_set_pin(2, GPIO_PV_HIGH);
}
void led_r_on()
{
  gpio_set_pin(1, GPIO_PV_HIGH);
}
void led_w_off()
{
  gpio_set_pin(2, GPIO_PV_LOW);
}
void led_r_off()
{
  gpio_set_pin(1, GPIO_PV_LOW);
}
static void camera_switch(void)
{
    g_camera_no = !g_camera_no;
    g_camera_no ? open_gc0328_0() : open_gc0328_1();

    int enable = g_camera_no ? 1 : 0;
    // irled(enable);
    led_r_on();
}

#if 0
static void draw_edge(uint32_t *gram, obj_info_t *obj_info, uint32_t index, uint16_t color)
{
  // uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t data = (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4, x1, y1, x2, y2;

    x1 = obj_info->obj[index].x1;
    y1 = obj_info->obj[index].y1;
    x2 = obj_info->obj[index].x2;
    y2 = obj_info->obj[index].y2;

    if (x1 <= 0)
        x1 = 1;
    if (x2 >= 319)
        x2 = 318;
    if (y1 <= 0)
        y1 = 1;
    if (y2 >= 239)
        y2 = 238;
    for(int x=x1,x<=x2,x++)
    {
      gram[y1]
    }
}
#else
static void draw_edge(uint32_t *gram, obj_info_t *obj_info, uint32_t index, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    // uint32_t data = (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4, x1, y1, x2, y2;

    x1 = obj_info->obj[index].x1;
    y1 = obj_info->obj[index].y1;
    x2 = obj_info->obj[index].x2;
    y2 = obj_info->obj[index].y2;

    if (x1 <= 0)
        x1 = 1;
    if (x2 >= 319)
        x2 = 318;
    if (y1 <= 0)
        y1 = 1;
    if (y2 >= 239)
        y2 = 238;

    addr1 = gram + (320 * y1 + x1) / 2;
    addr2 = gram + (320 * y1 + x2 - 8) / 2;
    addr3 = gram + (320 * (y2 - 1) + x1) / 2;
    addr4 = gram + (320 * (y2 - 1) + x2 - 8) / 2;
    for (uint32_t i = 0; i < 4; i++)
    {
        *addr1 = data;
        *(addr1 + 160) = data;
        *addr2 = data;
        *(addr2 + 160) = data;
        *addr3 = data;
        *(addr3 + 160) = data;
        *addr4 = data;
        *(addr4 + 160) = data;
        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }
    addr1 = gram + (320 * y1 + x1) / 2;
    addr2 = gram + (320 * y1 + x2 - 2) / 2;
    addr3 = gram + (320 * (y2 - 8) + x1) / 2;
    addr4 = gram + (320 * (y2 - 8) + x2 - 2) / 2;
    for (uint32_t i = 0; i < 8; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;
        addr1 += 160;
        addr2 += 160;
        addr3 += 160;
        addr4 += 160;
    }
}
#endif

void system_init(void)
{
  //初始化系统时钟
    sysctl_pll_set_freq(SYSCTL_PLL0, FREQ_PLL0_DEFAULT);
    sysctl_pll_set_freq(SYSCTL_PLL1, FREQ_PLL1_DEFAULT);
    sysctl_pll_set_freq(SYSCTL_PLL2, FREQ_PLL2_DEFAULT);
    //重新定义gpio引脚功能
    //串口初始化在_init_bsp
  //设置系统时钟
  sysctl_cpu_set_freq(FREQ_CPU_DEFAULT);
  //设置系统时钟阈值
  sysctl_clock_set_threshold(SYSCTL_THRESHOLD_AI, 0);
  //初始化dmac
  dmac_init();
  plic_init();
  printf("[F1] Pll0:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
  printf("[F1] Pll1:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
  printf("[F1] Pll2:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_PLL2));
  printf("[F1] cpu:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_CPU));
  printf("[F1] kpu:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_AI));
  //使能系统时钟
  sysctl_clock_enable(SYSCTL_CLOCK_AI);
  sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
  sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
  //使能系统中断
  sysctl_enable_irq();
  rtc_init();
  rtc_timer_set(2000, 1, 1, 0, 0, 0);
  // flash_init();
  w25qxx_init(3, 0);
  w25qxx_enable_quad_mode();

    /* Init SPI IO map and function settings */
  sysctl_set_spi0_dvp_data(1);

  //set LED
  fpioa_set_function(20,FUNC_GPIO1);//IR_LED
  fpioa_set_function(19,FUNC_GPIO2);//W_LED
  fpioa_set_function(18,FUNC_GPIO3);//SPK_CTL
  gpio_init();
  gpio_set_drive_mode(1, GPIO_DM_OUTPUT);
  gpio_pin_value_t value = GPIO_PV_HIGH;
  gpio_set_pin(1, value);
  gpio_set_drive_mode(2, GPIO_DM_OUTPUT);
  // gpio_set_pin(2, value);
}

// void setHostname()
// {
//   uint64_t machine_id = get_chip_number();
//     char hostname[20]={0};
//     snprintf(hostname,20,"%lx",machine_id);
//     esp8285_setHostname(hostname);
// }
void get_ip()
{
  ipconfig_obj data={0};
  get_ipconfig(&data);
  printf("ipconfig.IP=%s\n",data.ip);
  printf("ipconfig.gateway=%s\n",data.gateway);
  printf("ipconfig.netmask=%s\n",data.netmask);
  printf("ipconfig.ssid=%s\n",data.ssid);

}
/**
 * @brief  core1_function
 * @note   core1内核实现wifi联网配置功能、串口配置功能、播放语音
 * @param  *ctx: 
 * @retval 
 */
int core1_function(void *ctx)
{
    uint64_t core = current_coreid();
    
    printf("Core %ld Hello world\n", core);
    //初始化wifi
    esp8285_init();
    // setHostname();
    int work_mode = 1;
    while(1)
    {
      switch (work_mode)
      {
      case 0://join ap
         if(esp8285_joinAP(WIFI_SSID,WIFI_PASSWORD))
      {
        printf("join ap ok\r\n");
        work_mode = 1;
        get_ip();
      }
      
        break;
      case 1:
      break;
      
      default:
        break;
      }
     msleep(100);
    }
}
static void draw_key_point(uint32_t *gram, key_point_t *key_point, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr;

    for(uint32_t i = 0; i < 5; i++)
    {
        addr = gram + (320 * (key_point->point[i].y - 8) + key_point->point[i].x) / 2;
        *addr = data;
        *(addr + 160) = data;
    }
}
/**
 * @brief  按输入大小剪切图片
 * @note   
 * @param  src: 输入图像源 RGB565
 * @param  dst: 输出图像
 * @param  x1: 起始点x
 * @param  y1: 起始点y
 * @param  width: 剪切图像宽度
 * @param  height: 剪切图像高度
 * @retval None
 */
static void image_crop_RGB565(uint8_t *src,uint8_t *dst,uint32_t x1,uint32_t y1,uint16_t width, uint16_t height)
{
  //default width=240,height=240
  // //计算输入图像和输出图像偏移
  const int src_width = 320;
  int src_offset  = (src_width * y1 + x1)*2;//每个像素占2字节
  int dst_offset  = 0;
  
  // 遍历源图像，将像素值写入目标图像
  for (int j = 0; j < height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      // 读取源图像的像素值
      uint16_t pixel = *((uint16_t *)(src + src_offset));
      // 将像素值写入目标图像
      *(uint16_t *)(dst + dst_offset) = pixel;
      // 更新偏移量
      src_offset += 2;
      dst_offset += 2;
    }
    src_offset += (src_width-width)*2;
  }

  //增加图像旋转
  // const int src_width=320;
  // int dst_width = height;
  // int dst_height = width;
  // for (int j = 0; j < height; j++)
  // {
  //   for(int i=0;i<width;i++)
  //   {
  //     int src_offset = (y1+j)*src_width*2+(x1+i)*2;
  //     uint16_t pixel = *((uint16_t *)(src + src_offset));
  //     int dst_x = j;
  //     int dst_y = width-1-i;
  //     int dst_offset = dst_y*dst_width*2+dst_x*2;
  //     *(uint16_t *)(dst + dst_offset) = pixel;
  //   }
  // }
  
}
// #define KMODEL_SIZE (272 * 1024)
// uint8_t* model_data;

int main(void)
{
    system_init();
    uint64_t core = current_coreid();
    int data;
    printf("Core %ld Hello world\n", core);
    // printf("unique_id=%x\n",get_chip_number());//TODO::需调换端序 
    //打印软件版本号
    // LOGD("Software version:%s\r\n", SOFTWARE_VERSION);
    //读取flash中系统配置

    /* Clear stdin buffer before scanf */
    // sys_stdin_flush();

    // scanf("%d", &data);
    // LOGD("\nData is %d\n", data);
    //主程序实现识别程序
    //初始化flash
    // model_data = (uint8_t*)malloc(KMODEL_SIZE + 255);
    // uint8_t *model_data_align = (uint8_t*)(((uintptr_t)model_data+255)&(~255));
    // flash_read(0xA00000,KMODEL_SIZE,model_data_align);

    //初始化lcd
    lcd_init();
    lcd_set_rotation(1);
    // lcd_clear(BLACK);	
    // lcd_draw_string(136, 70, "DEMO", WHITE);
	  // lcd_draw_string(104, 150, "yolo face detection", WHITE);
    #if 0
    //test code
    char * str = "Hello world";
    lcd_draw_string(10,10,str,BLACK);
    // 显示固定图片内容
    // lcd_draw_picture(0,0,320,120,rgb_image);
    // lcd_draw_picture(0,120,320,120,rgb_image);
    #endif
    dvp_init(8);
    dvp_set_xclk_rate(24000000);
    // dvp_disable_burst();
    dvp_enable_burst();
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_output_enable(0, 1);//enable to AI
    dvp_set_output_enable(1, 1);//enable to lcd
    dvp_set_image_size(320, 240);

    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    /* set irq handle */
    plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, NULL);
    // //start
    // dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    // dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

    gc0328_init();
    open_gc0328_1();//红外
    // open_gc0328_0();//RGB
    kpu_image.pixel = 3;
    kpu_image.width = 320;
    // kpu_image.height = 240;
    kpu_image.height = 256;
    image_init(&kpu_image);
    dvp_set_ai_addr((uint32_t)kpu_image.addr + 320 * (256 * 0 + 8), (uint32_t)(kpu_image.addr + 320 * (256 * 1 + 8)), (uint32_t)(kpu_image.addr + 320 * (256 * 2 + 8)));
    // dvp_set_ai_addr((uint32_t)kpu_image.addr, (uint32_t)(kpu_image.addr + 320 * 240), (uint32_t)(kpu_image.addr + 320 * 240 * 2));
	  

    resized_image.pixel = 3;
    resized_image.width = 128;
    resized_image.height = 128;
    image_init(&resized_image);

    image_t show_image;
    show_image.pixel = 2;
    show_image.width = 240;
    show_image.height = 240;
    image_init(&show_image);

    display_image.pixel = 2;
    display_image.width = 320;
    display_image.height = 240;
    image_init(&display_image);
    dvp_set_display_addr((uint32_t)display_image.addr);
    dvp_disable_auto();

    /*ai init*/
    int ret = ai_init_license(license, 3);
    printf("ai_init_license ret:%d\n",ret);// OK return 0
    // ret = ai_init_model_address(0x200000, 0x300000, 0x400000, 0x500000, 0x600000);
    ret = ai_init_model_address(0x200000, 0x00400000, 0x00600000, 0x00800000, 0x00a00000);
    printf("ai_init_model_address ret:%d\n",ret);// OK return 0
    ret = ai_init_kpu_image_address(kpu_image.addr);
    printf("ai_init_kpu_image_address ret:%d\n",ret);// OK return 0
    ret = ai_load_model_all();
    printf("ai_load_model_all ret:%d\n",ret);// OK return 0
    // ai_init(model_data_align);
#if 0 //test code
        /* init face detect model */
    if (kpu_load_kmodel(&face_detect_task, model_data_align) != 0)
    {
        printf("\nmodel init error\n");//屏幕显示设备异常信息
        g_device_init_flag |= 0x01;
        while (1);
    }
    face_detect_rl.anchor_number = ANCHOR_NUM;
    face_detect_rl.anchor = anchor;
    face_detect_rl.threshold = 0.7;
    face_detect_rl.nms_value = 0.3;
    region_layer_init(&face_detect_rl, 20, 15, 30, kpu_image.width, kpu_image.height);
#endif
    //初始化定时器
    // timer_init(0);
    // timer_irq_register(0, 0, 0, 1, isr_tick, NULL);//执行定时任务
    // timer_set_interval(0, 0, 10000000);
    // timer_set_enable(0, 0, 1);	
        /* enable global interrupt */
    sysctl_enable_irq();
    /* system start */
    printf("System start\n");
      //注册core1内核
    // register_core1(core1_function, NULL);
    // led_w_on();
    float margin = 0.0f, face_thresh = FACE_RECGONITION_SCORE;
    uint32_t x1_tmp = 0,x2_tmp = 0,y1_tmp = 0,y2_tmp = 0;
    while(1)
    {
        //读取摄像头数据
        g_dvp_finish_flag = 0;
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
        while(g_dvp_finish_flag == 0)
            ;
        camera_switch();
        /* run face detect */
      ai_run_fd(&obj_detect_info);
      uint32_t face_found = 0;
        for(uint32_t i = 0; i < obj_detect_info.obj_number; i++)
        {
            x1_tmp = (uint32_t)obj_detect_info.obj[i].x1;
            x2_tmp = (uint32_t)obj_detect_info.obj[i].x2;
            y1_tmp = (uint32_t)obj_detect_info.obj[i].y1;
            y2_tmp = (uint32_t)obj_detect_info.obj[i].y2;
            if((y2_tmp-y1_tmp)<50)
              continue;
            image_absolute_src_resize(&kpu_image, &resized_image, &x1_tmp, &y1_tmp, &x2_tmp, &y2_tmp, margin, false);
            ai_run_keypoint(&kpu_image, &resized_image, x1_tmp, y1_tmp, x2_tmp, y2_tmp, &key_point);
            draw_key_point(display_image.addr, &key_point, RED);
            ai_run_feature(&resized_image, i);
            alive_tmp[i] = ai_run_alive(&kpu_image, &resized_image, obj_detect_info.obj[i].x1, obj_detect_info.obj[i].y1, obj_detect_info.obj[i].x2, obj_detect_info.obj[i].y2, &key_point);
        }
        /* run key point detect */
        // printf("kpu_run ret: %d face_detect_info.obj_number:%d\n",ret,face_detect_info.obj_number);
         for(uint32_t i = 0; i < obj_detect_info.obj_number; i++)
        {
            float score_max = 0;
            uint32_t score_index = 0;
            score_index = calulate_score(features_tmp[i], registered_face_features, MAX_REGISTERED_FACE_NUMBER, &score_max);
            if(score_max >= face_thresh)
            {
                draw_edge((uint32_t *)display_image.addr, &obj_detect_info, i, GREEN);
                if(alive_tmp[i] > 0 && g_camera_no == 1)
                {
                    sprintf(display_status, "%s_%s_%d", "recognized", "alive", score_index);
                } else
                {
                    sprintf(display_status, "%s_%d", "recognized", score_index);
                }
                ram_draw_string(display_image.addr, obj_detect_info.obj[i].x1, obj_detect_info.obj[i].y1 - 16, display_status, GREEN);
                printf("RECOGNIZED FACE: <%d> <%d> <%d> <%d>\n", obj_detect_info.obj[i].x1, obj_detect_info.obj[i].y1, obj_detect_info.obj[i].x2, obj_detect_info.obj[i].y2);
            } else
            {
                draw_edge((uint32_t *)display_image.addr, &obj_detect_info, i, RED);
                printf("features_tmp:");
                for (size_t i = 0; i < 192; i++)
                {
                  printf(" %f",features_tmp[0][i]);
                }
                printf("\n");
                
                printf("DETECTED FACE: <%d> <%d> <%d> <%d>\n", obj_detect_info.obj[i].x1, obj_detect_info.obj[i].y1, obj_detect_info.obj[i].x2, obj_detect_info.obj[i].y2);
            }
        }
                /* display pic*/
                 //裁剪图像中部240*240大小
                //转换图像方向 顺时针270度
        image_crop_RGB565(display_image.addr,show_image.addr,40,0,240,240);
        lcd_draw_picture(0, 40, 240, 240, (uint32_t *)show_image.addr);//显示图像位置
        // sprintf(display_status,"2024-04019 13:38");
        // lcd_draw_string(0, 0, display_status, WHITE);//显示时间 1s更新一次
        // lcd_draw_string(0, 0, display_status, WHITE);//显示识别状态
        // lcd_draw_string(245, 0, display_status, WHITE);//显示锁车状态
        // lcd_draw_string(245,0,display_status,BLUE);//显示识别人员姓名 中文
        // msleep(20);
    }
    return 0;
}
