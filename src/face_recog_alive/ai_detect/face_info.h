/*
 * @Author: Wangyg wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-12 11:44:03
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\face_info.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef __FACE_INFO_H
#define __FACE_INFO_H

#include "stdint.h"
#include "device_config.h"

//每一次大的更改配置结构的时候建议修改版本
#define CFG_VERSION (1.0f)
#define CFG_HEADER (0x55AA5558)

#define FEATURE_LEN 192

//flash 内数据存储默认8字节对齐
typedef struct _board_cfg
{
    uint8_t cfg_sha256[32]; //配置校验吗

    uint32_t header;            //配置头
    float version;                  //配置版本
    uint32_t cfg_right_flag;    //配置正确标志
    float fea_gate;                 //人脸比对阈值 默认79
    uint8_t seat_switch;            //座椅识别开关 1 on 0 off 默认0
    uint8_t relay_switch;           //继电器开关 1 on 0 off 默认1
    uint8_t acc_switch;             //acc识别开关 1：开启ACC识别开关   2：关闭ACC识别开关 默认0
    uint8_t seat_level;                 //座椅电平 1：高电平在位 0：低电平在位 默认1
    uint8_t can_type;                 //can协议类型 200：默认类型，不开启CAN
    uint8_t can_ctrl_switch;      //CAN控车开关
    uint32_t can_ctrl_interval;    //CAN锁车时长 默认10s
    // uint8_t can_break_switch;
    uint32_t seat_interval;
    uint32_t acc_interval;           //acc 断电锁车 默认10s

    char wifi_ssid[32];
    char wifi_passwd[32];
    char swVersion[32];
    // uint8_t user_custom_cfg[512];
}board_cfg_t __attribute__((aligned(8)));

typedef struct
{
    uint8_t stat; //0,rgb;1,ir;2,rgb+ir
    float fea_rgb[FEATURE_LEN];         //rgb特征
    // float fea_ir[FEATURE_LEN];
}face_fea_t;
typedef struct _face_info_t
{
    uint32_t index;                        //人脸对应的序号，同时用于校验人脸信息是否正确
    uint32_t uid;                           //人脸对应的uid
    uint8_t valid;                          //人脸特征是否有效
    face_fea_t info;                       //人脸特征信息
    int8_t name[NAME_LEN];     //人员姓名
}face_info_t;

typedef struct _face_save_info_t
{
    uint32_t header;                //数据头，用于校验数据是否存在
    uint32_t version;
    uint32_t number;              //存储的人脸数量
    uint32_t checksum;          //校验和
    uint32_t face_info_index[FACE_DATA_MAX_COUNT/32+1]; //每一位代表一个人脸特征

}face_save_info_t __attribute__((aligned(8)));
extern volatile face_save_info_t g_face_save_info;
extern volatile board_cfg_t g_board_cfg;

int flash_get_saved_feature_number(void);   //获取人脸特征数量
int flash_get_face_info(face_info_t *face_info, uint32_t id);
int flash_update_face_info(face_info_t *face_info);
int flash_get_saved_feature_sha256(uint8_t sha256[32]);
int flash_get_saved_uid_feature(uint32_t index, uint32_t *uid, face_fea_t *feature);// 获取执行需要人脸特征
uint32_t flash_get_id_by_uid(uint32_t uid);
int flash_delete_face_info(uint32_t id);
int flash_save_face_info(face_fea_t *features, uint32_t uid, uint32_t valid, char *name, uint32_t *ret_uid);
int flash_delete_face_all(void);
int flash_get_saved_faceinfo(face_info_t *info, uint32_t index);
void flash_load_face_info(void);
void flash_get_face_feature(float *reg_feature);
uint8_t flash_load_cfg(board_cfg_t *cfg);
uint8_t flash_save_cfg(board_cfg_t *cfg);
uint8_t flash_cfg_print(board_cfg_t *cfg);
uint8_t flash_cfg_set_default(board_cfg_t *cfg);
#endif // !__FACE_INFO_H
