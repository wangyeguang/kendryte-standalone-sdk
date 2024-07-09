/*
 * @Author: yeguang wang wangyeguang521@163.com
 * @Date: 2024-04-23 11:20:21
 * @LastEditors: Wangyg wangyeguang521@163.com
 * @LastEditTime: 2024-06-27 22:00:07
 * @FilePath: \kendryte-standalone-sdk-new\src\face_recog_alive\ai_detect\face_info.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "face_info.h"
#include "w25qxx.h"
#include "sha256.h"
#include "device_config.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "util.h"

volatile LOCK_IN_SECTION(DATA) face_save_info_t g_face_save_info;
volatile static LOCK_IN_SECTION(DATA) uint32_t uid_table[FACE_DATA_MAX_COUNT];  //人脸ID列表

/*face info function*/
/**
 * @brief  获取未被设置的faceid
 * @note   
 * @retval 
 */
static int get_face_id(void)
{
    int ret = -1;
    int i;
    for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if (((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1) == 0)
        {
            ret = i;
            break;
        }
    }

    if (i >= FACE_DATA_MAX_COUNT)
    {
        printf("get_face_id:> Err too many data\n");
        return -1;
    }
    return ret;
}

/**
 * @brief  初始化人脸ID列表
 * @note   
 * @param  init_flag: 
 * @retval None
 */
static void flash_init_uid_table(uint8_t init_flag)
{
    int i;
    uint32_t uid_addr;
    if (!init_flag)
    {
        for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
        {
            uid_addr = FACE_FEATURE_ADDR + i * sizeof(face_info_t) + 4;
            w25qxx_read_data(uid_addr, (uint8_t *)&uid_table[i], UID_LEN,W25QXX_QUAD_FAST);
        }
    }
    else
    {
        memset((void *)uid_table, 0, UID_LEN * FACE_DATA_MAX_COUNT);
    }
    return;
}
//高四位置0xF
/**
 * 用于查询UID是否存在
*/
uint32_t flash_get_id_by_uid(uint32_t uid)
{
    uint32_t i=0;
    for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if ((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            // if (memcmp(uid_table[i], uid, UID_LEN) == 0)
            if(uid == uid_table[i])
            {
                printf("find uid: %u\n",uid_table[i]);
                // for (uint8_t j = 0; j < UID_LEN; j++)
                //     printf("%02x ", uid_table[i]);
                return i | 0xF0000000;
            }
        }
    }
    return 0;
}
/**
 * @brief  删除指定ID人脸数据
 * @note   
 * @param  id: 
 * @retval 
 */
int flash_delete_face_info(uint32_t id)
{
    if (g_face_save_info.number == 0)
    {
        printf("del pic, no pic\n");
        return -1;
    }
    g_face_save_info.face_info_index[id / 32] &= ~(1 << (id % 32));
    g_face_save_info.number--;
    w25qxx_write_data(FACE_ADDR, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));
    // memset(uid_table[id], 0, UID_LEN);
    uid_table[id]=0;
    return 0;
}
/**
 * @brief  删除所有人脸数据
 * @note   
 * @retval 
 */

int flash_delete_face_all(void)
{
    g_face_save_info.number = 0;
    memset((void *)g_face_save_info.face_info_index, 0, sizeof(g_face_save_info.face_info_index));
    w25qxx_write_data(FACE_ADDR, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));
    memset((void *)uid_table, 0, UID_LEN * FACE_DATA_MAX_COUNT);
    return 0;
}


/**
 * @brief  保存人脸特征值
 * @note   
 * @param  *features: 
 * @param  uid: 
 * @param  valid: 
 * @param  *name: 
 * @param  *ret_uid: 
 * @retval 
 */
int flash_save_face_info(face_fea_t *features, uint32_t uid, uint32_t valid, char *name, uint32_t *ret_uid)
{
    face_info_t v_face_info;
    int face_id = get_face_id();
    if (face_id >= FACE_DATA_MAX_COUNT || face_id < 0)
    {
        printf("get_face_id err\n");
        return -1;
    }
    printf("Save face_id is %d\n", face_id);
    memcpy(&v_face_info.info, features, sizeof(v_face_info.info));
    v_face_info.index = face_id; //record to verify flash content

    if (uid == 0)
    {
        uint64_t tmp = read_cycle();
        memcpy((uint8_t *)&uid_table[face_id], (uint8_t *)&tmp, 8);
        memset((uint8_t *)&uid_table[face_id] + 8, 0, 8);
        if (ret_uid)
            memcpy(ret_uid, (uint32_t *)&uid_table[face_id], UID_LEN);
    }
    else
    {
        // memcpy(uid_table[face_id], uid, UID_LEN);
        uid_table[face_id] = uid;
    }
    // memcpy(v_face_info.uid, uid_table[face_id], UID_LEN);
    v_face_info.uid = uid_table[face_id];

    v_face_info.valid = valid; //pass permit

    if (name == NULL)
        strncpy((char *)v_face_info.name, "default", NAME_LEN);
    else
        strncpy((char *)v_face_info.name, name, NAME_LEN - 1);

    // if (note == NULL)
    //     strncpy(v_face_info.note, "null", NOTE_LEN - 1);
    // else
    //     strncpy(v_face_info.note, note, NOTE_LEN - 1);

    w25qxx_write_data(FACE_FEATURE_ADDR + face_id * (sizeof(face_info_t)), (uint8_t *)&v_face_info, sizeof(face_info_t));
    g_face_save_info.number++;
    g_face_save_info.face_info_index[face_id / 32] |= (1 << (face_id % 32));
    w25qxx_write_data(FACE_ADDR, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));

    //check is all face have ir feature
    // flash_all_face_have_ir_fea = flash_isall_have_ir();

    return 0;
}

/**
 * @brief  更新人脸特征值
 * @note   
 * @param  *face_info: 
 * @retval 
 */
int flash_update_face_info(face_info_t *face_info)
{
    w25qxx_write_data(FACE_FEATURE_ADDR + face_info->index * (sizeof(face_info_t)), (uint8_t *)face_info, sizeof(face_info_t));
    return 0;
}

/**
 * @brief  获取指定index人脸特征值及ID
 * @note   
 * @param  index: 
 * @param  *uid: 
 * @param  *feature: 
 * @retval 
 */
int flash_get_saved_uid_feature(uint32_t index, uint32_t *uid, face_fea_t *feature)
{
    uint32_t i;
    uint32_t cnt = 0;
    face_info_t v_face_info;
    for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if ((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if (cnt == index)
            {
                flash_get_face_info(&v_face_info, i);
                if (feature)
                    memcpy(feature, &v_face_info.info, sizeof(v_face_info.info));
                if (uid)
                    // memcpy(uid, v_face_info.uid, sizeof(v_face_info.uid));
                    *uid = v_face_info.uid;
                break;
            }
            else
            {
                cnt++;
            }
        }
    }

    if (cnt == index)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

#if 0
//1 success
//2 no feature store
//0 error
int flash_get_saved_feature_sha256(uint8_t  [32 + 1])
{
    static uint8_t flag = 0;
    float feature[192], tmp[192];
    uint32_t i = 0, cnt = 0;

    face_info_t v_face_info;
    face_fea_t v_info;

    flag = 0;
    cnt = 0;
    for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if ((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if (flash_get_face_info(&v_face_info, i) == 0)
            {
                // memcpy(tmp, v_face_info.info, (FEATURE_DIMENSION * 4));
                if (v_face_info.info.stat != 1)
                {
                    memcpy(tmp, v_face_info.info.fea_rgb, FEATURE_DIMENSION);
                }
                else
                {
                    printf("flash_get_face_info failed, no rgb feature\r\n");
                    return 0;
                }

                if (flag == 0)
                {
                    memcpy(feature, tmp, (FEATURE_DIMENSION));
                    flag = 1;
                }
                else
                {
                    for (uint16_t j = 0; j < (FEATURE_DIMENSION); j++)
                    {
                        feature[j] ^= tmp[j];
                    }
                }
            }
            else
            {
                printf("flash_get_face_info failed\r\n");
                return 0;
            }
            cnt++;
        }
    }

    if (cnt == 0)
    {
        printf("flash_get_saved_feature_sha256 cnt == 0\r\n");
        memset(sha256, 0xff, 32);
        sha256[32] = 0;
        return 1;
    }

    sha256_hard_calculate(feature, (FEATURE_DIMENSION), sha256);
    sha256[32] = 0;

#if 1
    printf("feature: \r\n");
    for (uint16_t i = 0; i < FEATURE_DIMENSION * 4; i++)
    {
        printf("%02x ", *(feature + i));
        if (i % 4 == 4)
            printf("\r\n");
    }
    printf("\r\n************************************\r\n");

    for (uint16_t i = 0; i < 32; i++)
    {
        printf("%02X", sha256[i]);
    }
    printf("\r\n");
#endif

    return 1;
}
#endif

int flash_get_saved_faceinfo(face_info_t *info, uint32_t index)
{
    uint32_t i;
    uint32_t cnt = 0;
    // face_info_t v_face_info;
    for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if ((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if (cnt == index)
            {
                flash_get_face_info(info, i);
                break;
            }
            else
            {
                cnt++;
            }
        }
    }
    return (i < FACE_DATA_MAX_COUNT) ? 0 : -1;
}

/**
 * @brief  获取存储的人脸特征数量
 * @note   
 * @retval 
 */
int flash_get_saved_feature_number(void)
{
    return g_face_save_info.number;
}

/**
 * @brief  获取指定ID人员信息
 * @note   
 * @param  *face_info: 
 * @param  id: 
 * @retval 
 */
int flash_get_face_info(face_info_t *face_info, uint32_t id)
{
    uint32_t image_address = FACE_FEATURE_ADDR + id * (sizeof(face_info_t));
    w25qxx_read_data(image_address, (uint8_t *)face_info, sizeof(face_info_t),W25QXX_QUAD_FAST);
    if (face_info->index != id)
    {
        printf("flash dirty! oft=%x, info.index %d != id %d\r\n", image_address, face_info->index, id);
        return -1;
    }
    return 0;
}
/**
 * @brief  获取指定index人脸特征值
 * @note   index对应人脸特征值列表下标及人脸信息列表下标
 * @param  *feature: 
 * @param  index: 
 * @retval 
 */
int flash_get_saved_feature(face_fea_t *feature, uint32_t index)
{
    uint32_t i;
    uint32_t cnt = 0;
    face_info_t v_face_info;
    for (i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if ((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if (cnt == index)
            {
                flash_get_face_info(&v_face_info, i);
                memcpy(feature, &v_face_info.info, sizeof(v_face_info.info));
                break;
            }
            else
            {
                cnt++;
            }
        }
    }
    return 0;
}
/**
 * @brief  加载人脸信息
 * @note   
 * @retval None
 */
void flash_load_face_info(void)
{
    w25qxx_read_data(FACE_ADDR,(uint8_t *)&g_face_save_info,sizeof(g_face_save_info),W25QXX_QUAD_FAST);
    if(g_face_save_info.header == FACE_HEADER)
    {
        printf("The header ok\r\n");
        uint32_t v_num = g_face_save_info.number;
        printf("there is %d img\r\n",v_num);
        v_num = 0;

        for(uint32_t i=0;i<FACE_DATA_MAX_COUNT;i++)
        {
            if(g_face_save_info.face_info_index[i/32]>>(i%32)&0x01)
            {
                v_num++;
            }
        }
         if (v_num != g_face_save_info.number)
        {
            printf("err:> index is %d, but saved is %d\r\n", v_num, g_face_save_info.number);
            g_face_save_info.number = v_num;
        }

        if (v_num >= FACE_DATA_MAX_COUNT)
        {
            printf("ERR, too many pic\r\n");
        }

        flash_init_uid_table(0);
    }
    else
    {
        printf("No header\r\n");
        g_face_save_info.header = FACE_HEADER;
        g_face_save_info.number = 0;
        memset((void *)g_face_save_info.face_info_index, 0, sizeof(g_face_save_info.face_info_index));
        w25qxx_write_data(FACE_ADDR, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));

        flash_init_uid_table(1);
    }
}
void flash_get_face_feature(float *reg_feature)
{
    face_info_t face_info;
    int ret=0;
    memset(reg_feature,0,(FACE_DATA_MAX_COUNT*FEATURE_LEN));
    for(int i=0;i<FACE_DATA_MAX_COUNT;i++)
    {
        // memset(&face_info,0,sizeof(face_info_t));
         if ((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
         {
            flash_get_face_info(&face_info, i);
            memcpy(reg_feature+i*FEATURE_LEN,face_info.info.fea_rgb,FEATURE_LEN);
         }
    }
    
}
/*device config function*/
uint8_t flash_load_cfg(board_cfg_t *cfg)
{
    w25qxx_status_t stat;
    uint8_t sha256[32];
     stat = w25qxx_read_data(BOARD_CFG_ADDR, (uint8_t *)cfg, sizeof(board_cfg_t),W25QXX_QUAD_FAST);

    if ((stat == W25QXX_OK) &&               /* 读flash正常 */
        (cfg->header == CFG_HEADER) &&       /* Header正确 */
        (cfg->version == (float)CFG_VERSION) /* Version正确*/
    )
    {
        sha256_hard_calculate((uint8_t *)cfg + 32, sizeof(board_cfg_t) - 32, sha256);

        if (memcmp(cfg->cfg_sha256, sha256, 32) == 0)
        {
            cfg->cfg_right_flag = 1;
            return 1;
        }
        else
        {
            printf("board cfg sha256 error\r\n");
            return 0;
        }
    }
    else
    {
        memset(cfg, 0, sizeof(board_cfg_t));
        return 0;
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     

    return 0;
}
/**
 * @brief  保存配置文件
 * @note   
 * @param  *cfg: 
 * @retval 
 */
uint8_t flash_save_cfg(board_cfg_t *cfg)
{
    w25qxx_status_t stat;

    cfg->cfg_right_flag = 0;

    sha256_hard_calculate((uint8_t *)cfg + 32, sizeof(board_cfg_t) - 32, cfg->cfg_sha256);

    printf("[ flash_save_cfg ] boardcfg checksum:");
    for (uint8_t i = 0; i < 32; i++)
    {
        printf("%02X", cfg->cfg_sha256[i]);
    }
    printf("\r\n");

    stat = w25qxx_write_data(BOARD_CFG_ADDR, (uint8_t *)cfg, sizeof(board_cfg_t));

    return (stat == W25QXX_OK) ? 1 : 0;
}

uint8_t flash_cfg_print(board_cfg_t *cfg)
{
    printf("dump board cfg:\r\n");
    printf("header:0x%X\r\n", cfg->header);
    printf("version:%f\r\n", cfg->version);
    printf("cfg_right_flag:%d\r\n", cfg->cfg_right_flag);
    printf("fea_gate:%f\r\n", cfg->fea_gate);
    printf("seat_switch:%d\r\n", cfg->seat_switch);
    printf("relay_switch:%d\r\n", cfg->relay_switch);
    printf("acc_switch:%d\r\n", cfg->acc_switch);
    printf("seat_level:%d\r\n", cfg->seat_level);
    printf("can_type:%d\r\n", cfg->can_type);
    printf("acc_interval:%d\r\n", cfg->acc_interval);
    printf("can_ctrl_switch:%d\r\n", cfg->can_ctrl_switch);
    printf("can_ctrl_interval:%d\r\n", cfg->can_ctrl_interval);
    printf("wifi_ssid:%s\r\n", cfg->wifi_ssid);
    printf("wifi_passwd:%s\r\n", cfg->wifi_passwd);
    return 0;
}

uint8_t flash_cfg_set_default(board_cfg_t *cfg)
{
     memset(cfg, 0, sizeof(board_cfg_t));
     cfg->header = CFG_HEADER;
    cfg->version = (float)CFG_VERSION;
    cfg->cfg_right_flag = 1;

    cfg->fea_gate = 79;                 //人脸比对阈值 默认79
    cfg->seat_switch = 0;            //座椅识别开关 1 on 0 off 默认0
    cfg->relay_switch = 1;           //继电器开关 1 on 0 off 默认1
    cfg->acc_switch = 0;             //acc识别开关 1：开启ACC识别开关   2：关闭ACC识别开关 默认0
    cfg->seat_level = 1;                 //座椅电平 1：高电平在位 0：低电平在位 默认1
    cfg->can_type  =200;                 //can协议类型 200：默认类型，不开启CAN
    cfg->acc_interval = 10;           //acc 断电锁车 默认10s
    cfg->can_ctrl_switch = 0;      //CAN控车开关
    cfg->can_ctrl_interval = 10;    //CAN锁车时长 默认10s
    
    strncpy(cfg->swVersion,SW_VERSION,32);
    memset(cfg->wifi_ssid, 0, sizeof(cfg->wifi_ssid));
    memset(cfg->wifi_passwd, 0, sizeof(cfg->wifi_passwd));
    // memcpy(cfg->wifi_ssid, "beitababa", strlen("beitababa"));
    // memcpy(cfg->wifi_passwd, "12345678", strlen("12345678"));
    return 0;
}