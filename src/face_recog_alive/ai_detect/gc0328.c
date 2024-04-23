/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS},
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include "gc0328.h"
#include "dvp.h"
#include "plic.h"
#include "sleep.h"
// #include "sensor.h"
// #include "mphalport.h"
#include "sipeed_i2c.h"
#include "fpioa.h"
#include "sleep.h"

static uint8_t sccb_reg_width = 8;
static int i2c_num = 0;
#define DCMI_RESET_LOW()      dvp->cmos_cfg &= ~DVP_CMOS_RESET
#define DCMI_RESET_HIGH()     dvp->cmos_cfg |= DVP_CMOS_RESET
#define DCMI_PWDN_LOW()       dvp->cmos_cfg &= ~DVP_CMOS_POWER_DOWN
#define DCMI_PWDN_HIGH()      dvp->cmos_cfg |= DVP_CMOS_POWER_DOWN


static const uint8_t sensor_rgb_regs[][2] =
{
    {0xFE, 0x80},   // [7] soft reset; [1:0] page select 00:REGF 01:REGF1
    {0xFE, 0x80},
    {0xFC, 0x16},   // [4] digital clock enable; [2] da25_en; [1] da18_en
    {0xFC, 0x16},
    {0xFC, 0x16},
    {0xFC, 0x16},

    {0xFE, 0x00},   // [7] soft reset; [1:0] page select 00:REGF 01:REGF1
    {0x4F, 0x00},   // [0] AEC enable
    {0x42, 0x00},   // [7] auto saturation; [6] auto EE; [5] auto DN; [4] auto DD; [3] auto LSC; [2] ABS enable; [1] AWB enable; [0] auto Y offset
    {0x03, 0x00},   // Exposure time high: [3:0] exposure [11:8], use line processing time as the unit. controlled by AEC if AEC is in function
    {0x04, 0xC0},   // Exposure time low: Exposure[7:0], controlled by AEC if AEC is in function
    {0x77, 0x62},   // AWB_R_gain: 2.6bits, AWB red gain, controlled by AWB
    {0x78, 0x40},   // AWB_G_gain: 2.6bits, AWB green gain, controlled by AWB
    {0x79, 0x4D},   // AWB_B_gain: 2.6bits, AWB blue gain, controlled by AWB
        
    {0x05, 0x02},   // HB high: [3:0] HBLANK high bit[11:8]
    {0x06, 0x2C},   // HB low: HBLANK low bit[7:0]
    {0x07, 0x00},   // VB high: [3:0] VBLANK high bit[11:8]
    {0x08, 0xB8},   // VB low: VBLANK low bit[7:0]
    
    {0xFE , 0x01},   // page1: [7] soft reset; [1:0] page select 00:REGF 01:REGF1
    {0x29 , 0x00},   // AEC_anti_flicker_step[11:8]: Anti-flicker step[11:8]
    {0x2a , 0x60},   // AEC_anti_flicker_step[7:0]: Anti-flicker step[7:0]
        
    {0x2b , 0x05}, //{0x2b , 0x02},   // AEC_exp_level_0
    {0x2c , 0x1E}, //{0x2c , 0xA0},            
    {0x2D , 0x05}, //{0x2D , 0x03},   // AEC_exp_level_1
   {0x2E , 0x1E},//{0x2E , 0x00},   
   {0x2F , 0x05}, // {0x2F , 0x03},   // AEC_exp_level_2
   {0x30 , 0x1E},   // {0x30 , 0xC0},   
    {0x31 , 0x05},   // AEC_exp_level_3 
    {0x32 , 0x1E}, // {0x32 , 0x40},   
    {0xFE , 0x00},   // page 0

	// {0x2b, 0x02},	//{0x2b , 0x02},   // AEC_exp_level_0
	// {0x2c, 0x00},	//{0x2c , 0xA0},     
	// {0x2d, 0x02},	//{0x2D , 0x03},   // AEC_exp_level_1
	// {0x2e, 0x00},	//{0x2E , 0x00}, 
	// {0x2f, 0x02},	//{0x2F , 0x03},   // AEC_exp_level_2
	// {0x30, 0x00},	//{0x30 , 0xC0}, 
	// {0x31, 0x02},	//{0x31 , 0x05},   // AEC_exp_level_3 
	// {0x32, 0x00},	//{0x32 , 0x40}, 
    
    {0xFE , 0x01},   // page 1
    {0x51 , 0x80},   // AWB_PRE_THD_min: Dominate luma THD
    {0x52 , 0x12},   
    {0x53 , 0x80},   // AWB_PRE_THD_min_MIX: mix luma number THD
    {0x54 , 0x60},   
    {0x55 , 0x01},   
    {0x56 , 0x06},   
    {0x5B , 0x02},   // mix base gain and adaptive gain limit
    {0x61 , 0xDC}, 
    {0x62 , 0xDC},  
    {0x7C , 0x71},   // adust_speed adjust_margin: [6:4] AWB gain adjust speed, the bigger the quicker; [3:0] if averages of R/G/B's difference is smaller than margin, it means AWB is OK, and AWB will stop
    {0x7D , 0x00},  
    {0x76 , 0x00},  
    {0x79 , 0x20},  
    {0x7B , 0x00},  // show_and_mode: [5] skin_mode; [1] dark_mode
    {0x70 , 0xFF},  
    {0x71 , 0x00},  
    {0x72 , 0x10},  
    {0x73 , 0x40},  
    {0x74 , 0x40},  
        
    {0x50 , 0x00},  
    {0xFE , 0x01},  // page 1
    {0x4F , 0x00},   
    {0x4C , 0x01},  
    {0x4F , 0x00},  
    {0x4F , 0x00},  
    {0x4F , 0x00},  
    {0x4D , 0x36},  
    {0x4E , 0x02}, 
    {0x4E , 0x02},  
    {0x4D , 0x44}, 
    {0x4E , 0x02},
    {0x4E , 0x02},
    {0x4E , 0x02},
    {0x4E , 0x02},
    {0x4D , 0x53},  
    {0x4E , 0x08},  
    {0x4E , 0x08},  
    {0x4E , 0x02}, 
    {0x4D , 0x63},
    {0x4E , 0x08},
    {0x4E , 0x08},
    {0xFE , 0x00},  // page 0
    {0x4D , 0x73},  // auto_middle_gamma_en: [0] auto middle gamma enable
    {0x4E , 0x20},  
    {0x4D , 0x83},
    {0x4E , 0x20},
    {0x4F , 0x01},  // AEC enable: [0] AEC enable

    {0x50 , 0x88},  // Crop_win_mode: [0] crop window mode enable
    {0xFE , 0x00},  // page 0
        
    {0x27 , 0x00},  
    {0x2A , 0x40},
    {0x2B , 0x40},
    {0x2C , 0x40},
    {0x2D , 0x40},

    {0xFE , 0x00},  // page 0
    {0x0D, 0x01}, // window height high: [0] Window height high[8]   -- height 488
    {0x0E, 0xE8}, // Window height low: Window height low[7:0]
    {0x0F, 0x02}, // window width high: [1:0] Window width high[9:8]   -- width 648
    {0x10, 0x88}, // window width low: Window width low[7:0]
    {0x09 , 0x00},  // Row start high: [0] row start high bit[8]
    {0x0A , 0x00},  // Row start low:row start low bit[7:0]
    {0x0B , 0x00},  // Col start high: [1:0] col start high bit[9:8]
    {0x0C , 0x00},  // Col start low: col start low bit[7:0]
    {0x16 , 0x00},  // Analog gain: [7] Analog gain enable
	{0x17 , 0x15}, //{0x17 , 0x16},//控制翻转[:1]和镜像[:0]
    {0x18 , 0x0E},  // CISCTL_mode2: [7:6] output mode-VGA mode; [5] column binning; [4] double reset mode; [3:2] sdark mode -- sdark 4 rows in each frame; [1] new exposure/normal bad frame; [0] badframe enable
    {0x19 , 0x06},  // CISCTL_mode3: [6] for double restg; [5] restg on/off; [4] capture AD data edge; [3:0] AD pipe number

    {0x1B , 0x48},  // Rsh width: [7:4] restg_width, X2; [3:0] sh_width, X2
    {0x1F , 0xC8},  
    {0x20 , 0x01},
    {0x21 , 0x78},
    {0x22 , 0xB0},
    {0x23 , 0x04},
    {0x24 , 0x11},
    {0x26 , 0x00},

    {0x50 , 0x01},  // Crop_win_mode: [0] crop window mode enable

    {0x59 , 0x22},  // subsample: [7:4] subsample row ratio; [3:0] subsample col ratio
    {0x51 , 0x00},  // Crop_win_y1
    {0x52 , 0x00},
    {0x53 , 0x00},  // Crop_win_x1
    {0x54 , 0x00},
    {0x55 , 0x00},  // Crop_win_height
    {0x56 , 0xF0},  
    {0x57 , 0x01},  // Crop_win_width
    {0x58 , 0x40},

    {0x70 , 0x85},  // Global gain

    {0x40 , 0x7F},  // Block_enable_1: [7] middle gamma enable; [6] gamma enable; [5] CC enable; [4] Edge enhancement enable, [3] Interpolation enable; [2] Noise removal enable; [1] Defect removal enable; [0] Lens-shading correction enable
    {0x41 , 0x26},  // Block_enable_2: [6] low light Y enable; [5] skin enable; [4] skin Y enable; [3] new skin mode; [2] autogray enable; [1] Y gamma enable; [0] block skin
    {0x42 , 0xFF},  // [7] auto saturation; [6] auto EE; [5] auto DN; [4] auto DD; [3] auto LSC; [2] ABS enable; [1] AWB enable; [0] auto Y offset
    {0x45 , 0x00},  // Auto middle gamma mode: [1] auto gamma mode outdoor; [0] auto gamma mode lowlight
    {0x44 , 0x06}, //{0x44 , 0x06},  // Output_format: RGB565 0x40 YUV
    {0x46 , 0x02},  // SYNC_mode: [1] HSYNC polarity; [0] VSYNC polarity

    {0x4B , 0x01},  // Debug mode 1: [0] update gain mode
    {0x50 , 0x01},  // Crop_win_mode: [0] crop window mode enable

    {0x7E , 0x0A},   
    {0x7F , 0x03},
    {0x80 , 0x27},
    {0x81 , 0x15},
    {0x82 , 0x90},
    {0x83 , 0x02},
    {0x84 , 0x23},
    {0x90 , 0x2C},
    {0x92 , 0x02},
    {0x94 , 0x02},
    {0x95 , 0x35},

    {0xD1 , 0x32},  // Cb saturation
    {0xD2 , 0x32},  // Cr saturation
    {0xDD , 0x18},  
    {0xDE , 0x32},
    {0xE4 , 0x88},
    {0xE5 , 0x40},
    {0xD7 , 0x0E},

	//RGB GAMMA
    {0xFE , 0x00},  // page 0
    {0xBF , 0x10},
    {0xc0, 0x1C},//{0xC0 , 0x1C},//
    {0xC1 , 0x33},
    {0xC2 , 0x48},
    {0xC3 , 0x5A},
    {0xC4 , 0x6B},
    {0xC5 , 0x7B},
    {0xC6 , 0x95},
    {0xC7 , 0xAB},
    {0xC8 , 0xBF},
    {0xC9 , 0xCD},
    {0xCA , 0xD9},
    {0xCB , 0xE3},
    {0xCC , 0xEB},
    {0xCD , 0xF7},
    {0xCE , 0xFD},
    {0xCF , 0xFF},
	

	// {0xfe , 0x00},
	// 	{0xBF, 0x10},
	// 	{0xc0, 0x20},
	// 	{0xc1, 0x38},
	// 	{0xc2, 0x4E},
	// 	{0xc3, 0x63},
	// 	{0xc4, 0x76},
	// 	{0xc5, 0x87},
	// 	{0xc6, 0xA2},
	// 	{0xc7, 0xB8},
	// 	{0xc8, 0xCA},
	// 	{0xc9, 0xD8},
	// 	{0xcA, 0xE3},
	// 	{0xcB, 0xEB},
	// 	{0xcC, 0xF0},
	// 	{0xcD, 0xF8},
	// 	{0xcE, 0xFD},
	// 	{0xcF, 0xFF},
		
		//Y Gamma
    // {0xFE , 0x00},  // page 0
    // {0x63 , 0x00},
    // {0x64 , 0x05},
    // {0x65 , 0x0C},
    // {0x66 , 0x1A},
    // {0x67 , 0x29},
    // {0x68 , 0x39},
    // {0x69 , 0x4B},
    // {0x6A , 0x5E},
    // {0x6B , 0x82},
    // {0x6C , 0xA4},
    // {0x6D , 0xC5},
    // {0x6E , 0xE5},
    // {0x6F , 0xFF},
	//0.9  //RGBsensor
	{0xfe , 0x00},
	{0x63,0x0 },
	{0x64,0x1B},
	{0x65,0x33},
	{0x66,0x49},
	{0x67,0x5F},
	{0x68,0x74},
	{0x69,0x89},
	{0x6a,0x9D},
	{0x6b,0xB1},
	{0x6c,0xC5},
	{0x6d,0xD9},
	{0x6e,0xEC},
	{0x6f,0xff},
		//0.5  //IR sensor
	// {0xfe , 0x00},  
	// {0x63 , 0x00},  
	// {0x64 , 0x49},  
	// {0x65 , 0x68},  
	// {0x66 , 0x80},  
	// {0x67 , 0x93},  
	// {0x68 , 0xa5},  
	// {0x69 , 0xb5},  
	// {0x6a , 0xc3},  
	// {0x6b , 0xd1},  
	// {0x6c , 0xdd},  
	// {0x6d , 0xe9},  
	// {0x6e , 0xf5},  
	// {0x6f , 0xFF},

    {0xFE , 0x01},  // page 1
    {0x18 , 0x02},
    {0xFE , 0x00},  // page 0
    {0x98 , 0x00},
    {0x9B , 0x20},
    {0x9C , 0x80},
    {0xA4 , 0x10},
    {0xA8 , 0xB0},
    {0xAA , 0x40},
    {0xA2 , 0x23},
    {0xAD , 0x01},

    {0xFE , 0x01},  // page 1
    {0x9C , 0x02},
    {0x08 , 0xA0},
    {0x09 , 0xE8},

    {0x10 , 0x00},
    {0x11 , 0x11},
    {0x12 , 0x10},
    {0x13 , 0x80},
    {0x15 , 0xFC},
    {0x18 , 0x03},
    {0x21 , 0xC0},
    {0x22 , 0x60},
    {0x23 , 0x30},
    {0x25 , 0x00},
    {0x24 , 0x14},

    {0xFE , 0x01},  // page 1
    {0xC0 , 0x10},
    {0xC1 , 0x0C},
    {0xC2 , 0x0A},
    {0xC6 , 0x0E},
    {0xC7 , 0x0B},
    {0xC8 , 0x0A},
    {0xBA , 0x26},
    {0xBB , 0x1C},
    {0xBC , 0x1D},
    {0xB4 , 0x23},
    {0xB5 , 0x1C},
    {0xB6 , 0x1A},
    {0xC3 , 0x00},
    {0xC4 , 0x00},
    {0xC5 , 0x00},
    {0xC9 , 0x00},
    {0xCA , 0x00},
    {0xCB , 0x00},
    {0xBD , 0x00},
    {0xBE , 0x00},
    {0xBF , 0x00},
    {0xB7 , 0x07},
    {0xB8 , 0x05},
    {0xB9 , 0x05},
    {0xA8 , 0x07},
    {0xA9 , 0x06},
    {0xAA , 0x00},
    {0xAB , 0x04},
    {0xAC , 0x00},
    {0xAD , 0x02},
    {0xAE , 0x0D},
    {0xAF , 0x05},
    {0xB0 , 0x00},
    {0xB1 , 0x07},
    {0xB2 , 0x03},
    {0xB3 , 0x00},
    {0xA4 , 0x00},
    {0xA5 , 0x00},
    {0xA6 , 0x00},
    {0xA7 , 0x00},
    {0xA1 , 0x3C},
    {0xA2 , 0x50},
    {0xFE , 0x00},  // page 0

    {0xB1 , 0x04},  
    {0xB2 , 0xFD},
    {0xB3 , 0xFC},
    {0xB4 , 0xF0},
    {0xB5 , 0x05},
    {0xB6 , 0xF0},

    // msleep(200);
    {0xFE , 0x00},  // page 0
    {0x27 , 0xF7},
    {0x28 , 0x7F},
    {0x29 , 0x20},
    {0x33 , 0x20},
    {0x34 , 0x20},
    {0x35 , 0x20},
    {0x36 , 0x20},
    {0x32 , 0x08},

    {0x47 , 0x00},
    {0x48 , 0x00},

    {0xFE , 0x01},  // page 1
    {0x79 , 0x00},  
    {0x7D , 0x00},
    {0x50 , 0x88},  // AWB_PRE_mode: [7] PRE_enable; [3] AWB_PRE_adjust_speed enable
    {0x5B , 0x0C},  
    {0x76 , 0x8F},
    {0x80 , 0x70},
    {0x81 , 0x70},
    {0x82 , 0xB0},
    {0x70 , 0xFF},
    {0x71 , 0x00},
    {0x72 , 0x28},
    {0x73 , 0x0B},
    {0x74 , 0x0B},

    {0xFE , 0x00},  // page 0
    {0x70 , 0x45},  // global gain
    {0x4F , 0x01},  // AEC enable

    // {0xF1 , 0x00},  // Pad_setting1: [2] pclk output enable; [1] HSYNC output enable; [0] VSYNC output enable
    // {0xF2 , 0x00},  // Pad_setting2: [0] data output enable
    {0xF1 , 0x00}, 
    {0xF2 , 0x01},
    {0 , 0}
};

static const uint8_t sensor_ir_regs[][2] =
{
    {0xFE, 0x80},   // [7] soft reset; [1:0] page select 00:REGF 01:REGF1
    {0xFE, 0x80},
    {0xFC, 0x16},   // [4] digital clock enable; [2] da25_en; [1] da18_en
    {0xFC, 0x16},
    {0xFC, 0x16},
    {0xFC, 0x16},

    {0xFE, 0x00},   // [7] soft reset; [1:0] page select 00:REGF 01:REGF1
    {0x4F, 0x00},   // [0] AEC enable
    {0x42, 0x00},   // [7] auto saturation; [6] auto EE; [5] auto DN; [4] auto DD; [3] auto LSC; [2] ABS enable; [1] AWB enable; [0] auto Y offset
    {0x03, 0x00},   // Exposure time high: [3:0] exposure [11:8], use line processing time as the unit. controlled by AEC if AEC is in function
    {0x04, 0xC0},   // Exposure time low: Exposure[7:0], controlled by AEC if AEC is in function
    {0x77, 0x62},   // AWB_R_gain: 2.6bits, AWB red gain, controlled by AWB
    {0x78, 0x40},   // AWB_G_gain: 2.6bits, AWB green gain, controlled by AWB
    {0x79, 0x4D},   // AWB_B_gain: 2.6bits, AWB blue gain, controlled by AWB
        
    {0x05, 0x02},   // HB high: [3:0] HBLANK high bit[11:8]
    {0x06, 0x2C},   // HB low: HBLANK low bit[7:0]
    {0x07, 0x00},   // VB high: [3:0] VBLANK high bit[11:8]
    {0x08, 0xB8},   // VB low: VBLANK low bit[7:0]
    
    {0xFE , 0x01},   // page1: [7] soft reset; [1:0] page select 00:REGF 01:REGF1
    {0x29 , 0x00},   // AEC_anti_flicker_step[11:8]: Anti-flicker step[11:8]
    {0x2a , 0x60},   // AEC_anti_flicker_step[7:0]: Anti-flicker step[7:0]
        
    {0x2b , 0x02},   // AEC_exp_level_0
    {0x2c , 0xA0},            
    {0x2D , 0x03},   // AEC_exp_level_1
   {0x2E , 0x00},   
    {0x2F , 0x03},   // AEC_exp_level_2
   {0x30 , 0xC0},   
    {0x31 , 0x05},   // AEC_exp_level_3 
    {0x32 , 0x40},   
    {0xFE , 0x00},   // page 0

	// {0x2b, 0x02},	//{0x2b , 0x02},   // AEC_exp_level_0
	// {0x2c, 0x00},	//{0x2c , 0xA0},     
	// {0x2d, 0x02},	//{0x2D , 0x03},   // AEC_exp_level_1
	// {0x2e, 0x00},	//{0x2E , 0x00}, 
	// {0x2f, 0x02},	//{0x2F , 0x03},   // AEC_exp_level_2
	// {0x30, 0x00},	//{0x30 , 0xC0}, 
	// {0x31, 0x02},	//{0x31 , 0x05},   // AEC_exp_level_3 
	// {0x32, 0x00},	//{0x32 , 0x40}, 
    
    {0xFE , 0x01},   // page 1
    {0x51 , 0x80},   // AWB_PRE_THD_min: Dominate luma THD
    {0x52 , 0x12},   
    {0x53 , 0x80},   // AWB_PRE_THD_min_MIX: mix luma number THD
    {0x54 , 0x60},   
    {0x55 , 0x01},   
    {0x56 , 0x06},   
    {0x5B , 0x02},   // mix base gain and adaptive gain limit
    {0x61 , 0xDC}, 
    {0x62 , 0xDC},  
    {0x7C , 0x71},   // adust_speed adjust_margin: [6:4] AWB gain adjust speed, the bigger the quicker; [3:0] if averages of R/G/B's difference is smaller than margin, it means AWB is OK, and AWB will stop
    {0x7D , 0x00},  
    {0x76 , 0x00},  
    {0x79 , 0x20},  
    {0x7B , 0x00},  // show_and_mode: [5] skin_mode; [1] dark_mode
    {0x70 , 0xFF},  
    {0x71 , 0x00},  
    {0x72 , 0x10},  
    {0x73 , 0x40},  
    {0x74 , 0x40},  
        
    {0x50 , 0x00},  
    {0xFE , 0x01},  // page 1
    {0x4F , 0x00},   
    {0x4C , 0x01},  
    {0x4F , 0x00},  
    {0x4F , 0x00},  
    {0x4F , 0x00},  
    {0x4D , 0x36},  
    {0x4E , 0x02}, 
    {0x4E , 0x02},  
    {0x4D , 0x44}, 
    {0x4E , 0x02},
    {0x4E , 0x02},
    {0x4E , 0x02},
    {0x4E , 0x02},
    {0x4D , 0x53},  
    {0x4E , 0x08},  
    {0x4E , 0x08},  
    {0x4E , 0x02}, 
    {0x4D , 0x63},
    {0x4E , 0x08},
    {0x4E , 0x08},
    {0xFE , 0x00},  // page 0
    {0x4D , 0x73},  // auto_middle_gamma_en: [0] auto middle gamma enable
    {0x4E , 0x20},  
    {0x4D , 0x83},
    {0x4E , 0x20},
    {0x4F , 0x01},  // AEC enable: [0] AEC enable

    {0x50 , 0x88},  // Crop_win_mode: [0] crop window mode enable
    {0xFE , 0x00},  // page 0
        
    {0x27 , 0x00},  
    {0x2A , 0x40},
    {0x2B , 0x40},
    {0x2C , 0x40},
    {0x2D , 0x40},

    {0xFE , 0x00},  // page 0
    {0x0D, 0x01}, // window height high: [0] Window height high[8]   -- height 488
    {0x0E, 0xE8}, // Window height low: Window height low[7:0]
    {0x0F, 0x02}, // window width high: [1:0] Window width high[9:8]   -- width 648
    {0x10, 0x88}, // window width low: Window width low[7:0]
    {0x09 , 0x00},  // Row start high: [0] row start high bit[8]
    {0x0A , 0x00},  // Row start low:row start low bit[7:0]
    {0x0B , 0x00},  // Col start high: [1:0] col start high bit[9:8]
    {0x0C , 0x00},  // Col start low: col start low bit[7:0]
    {0x16 , 0x00},  // Analog gain: [7] Analog gain enable
	{0x17 , 0x15}, //{0x17 , 0x16},//控制翻转[:1]和镜像[:0]
    {0x18 , 0x0E},  // CISCTL_mode2: [7:6] output mode-VGA mode; [5] column binning; [4] double reset mode; [3:2] sdark mode -- sdark 4 rows in each frame; [1] new exposure/normal bad frame; [0] badframe enable
    {0x19 , 0x06},  // CISCTL_mode3: [6] for double restg; [5] restg on/off; [4] capture AD data edge; [3:0] AD pipe number

    {0x1B , 0x48},  // Rsh width: [7:4] restg_width, X2; [3:0] sh_width, X2
    {0x1F , 0xC8},  
    {0x20 , 0x01},
    {0x21 , 0x78},
    {0x22 , 0xB0},
    {0x23 , 0x04},
    {0x24 , 0x11},
    {0x26 , 0x00},

    {0x50 , 0x01},  // Crop_win_mode: [0] crop window mode enable

    {0x59 , 0x22},  // subsample: [7:4] subsample row ratio; [3:0] subsample col ratio
    {0x51 , 0x00},  // Crop_win_y1
    {0x52 , 0x00},
    {0x53 , 0x00},  // Crop_win_x1
    {0x54 , 0x00},
    {0x55 , 0x00},  // Crop_win_height
    {0x56 , 0xF0},  
    {0x57 , 0x01},  // Crop_win_width
    {0x58 , 0x40},

    {0x70 , 0x85},  // Global gain

    {0x40 , 0x7F},  // Block_enable_1: [7] middle gamma enable; [6] gamma enable; [5] CC enable; [4] Edge enhancement enable, [3] Interpolation enable; [2] Noise removal enable; [1] Defect removal enable; [0] Lens-shading correction enable
    {0x41 , 0x26},  // Block_enable_2: [6] low light Y enable; [5] skin enable; [4] skin Y enable; [3] new skin mode; [2] autogray enable; [1] Y gamma enable; [0] block skin
    {0x42 , 0xFF},  // [7] auto saturation; [6] auto EE; [5] auto DN; [4] auto DD; [3] auto LSC; [2] ABS enable; [1] AWB enable; [0] auto Y offset
    {0x45 , 0x00},  // Auto middle gamma mode: [1] auto gamma mode outdoor; [0] auto gamma mode lowlight
    {0x44 , 0x06}, //{0x44 , 0x06},  // Output_format: RGB565 0x40 YUV
    {0x46 , 0x02},  // SYNC_mode: [1] HSYNC polarity; [0] VSYNC polarity

    {0x4B , 0x01},  // Debug mode 1: [0] update gain mode
    {0x50 , 0x01},  // Crop_win_mode: [0] crop window mode enable

    {0x7E , 0x0A},   
    {0x7F , 0x03},
    {0x80 , 0x27},
    {0x81 , 0x15},
    {0x82 , 0x90},
    {0x83 , 0x02},
    {0x84 , 0x23},
    {0x90 , 0x2C},
    {0x92 , 0x02},
    {0x94 , 0x02},
    {0x95 , 0x35},

    {0xD1 , 0x32},  // Cb saturation
    {0xD2 , 0x32},  // Cr saturation
    {0xDD , 0x18},  
    {0xDE , 0x32},
    {0xE4 , 0x88},
    {0xE5 , 0x40},
    {0xD7 , 0x0E},

	//RGB GAMMA
    // {0xFE , 0x00},  // page 0
    // {0xBF , 0x10},
    // {0xc0, 0x1C},//{0xC0 , 0x1C},//
    // {0xC1 , 0x33},
    // {0xC2 , 0x48},
    // {0xC3 , 0x5A},
    // {0xC4 , 0x6B},
    // {0xC5 , 0x7B},
    // {0xC6 , 0x95},
    // {0xC7 , 0xAB},
    // {0xC8 , 0xBF},
    // {0xC9 , 0xCD},
    // {0xCA , 0xD9},
    // {0xCB , 0xE3},
    // {0xCC , 0xEB},
    // {0xCD , 0xF7},
    // {0xCE , 0xFD},
    // {0xCF , 0xFF},
	

	// {0xfe , 0x00},
	// 	{0xBF, 0x10},
	// 	{0xc0, 0x20},
	// 	{0xc1, 0x38},
	// 	{0xc2, 0x4E},
	// 	{0xc3, 0x63},
	// 	{0xc4, 0x76},
	// 	{0xc5, 0x87},
	// 	{0xc6, 0xA2},
	// 	{0xc7, 0xB8},
	// 	{0xc8, 0xCA},
	// 	{0xc9, 0xD8},
	// 	{0xcA, 0xE3},
	// 	{0xcB, 0xEB},
	// 	{0xcC, 0xF0},
	// 	{0xcD, 0xF8},
	// 	{0xcE, 0xFD},
	// 	{0xcF, 0xFF},
		//Gamma for night mode
		{0xfe , 0x00},
		{0xBF, 0x0B},
		{0xc0, 0x16},
		{0xc1, 0x29},
		{0xc2, 0x3C},
		{0xc3, 0x4F},
		{0xc4, 0x5F},
		{0xc5, 0x6F},
		{0xc6, 0x8A},
		{0xc7, 0x9F},
		{0xc8, 0xB4},
		{0xc9, 0xC6},
		{0xcA, 0xD3},
		{0xcB, 0xDD},
		{0xcC, 0xE5},
		{0xcD, 0xF1},
		{0xcE, 0xFA},
		{0xcF, 0xFF},
		//Y Gamma
    // {0xFE , 0x00},  // page 0
    // {0x63 , 0x00},
    // {0x64 , 0x05},
    // {0x65 , 0x0C},
    // {0x66 , 0x1A},
    // {0x67 , 0x29},
    // {0x68 , 0x39},
    // {0x69 , 0x4B},
    // {0x6A , 0x5E},
    // {0x6B , 0x82},
    // {0x6C , 0xA4},
    // {0x6D , 0xC5},
    // {0x6E , 0xE5},
    // {0x6F , 0xFF},
	//0.9  //RGBsensor
	// {0xfe , 0x00},
	// {0x63,0x0 },
	// {0x64,0x1B},
	// {0x65,0x33},
	// {0x66,0x49},
	// {0x67,0x5F},
	// {0x68,0x74},
	// {0x69,0x89},
	// {0x6a,0x9D},
	// {0x6b,0xB1},
	// {0x6c,0xC5},
	// {0x6d,0xD9},
	// {0x6e,0xEC},
	// {0x6f,0xff},
		//0.5  //IR sensor
	{0xfe , 0x00},  
	{0x63 , 0x00},  
	{0x64 , 0x49},  
	{0x65 , 0x68},  
	{0x66 , 0x80},  
	{0x67 , 0x93},  
	{0x68 , 0xa5},  
	{0x69 , 0xb5},  
	{0x6a , 0xc3},  
	{0x6b , 0xd1},  
	{0x6c , 0xdd},  
	{0x6d , 0xe9},  
	{0x6e , 0xf5},  
	{0x6f , 0xFF},

	// {0xfe , 0x00},
	// {0x63,0   },
	// {0x64,0x10},
	// {0x65,0x1c},
	// {0x66,0x30},
	// {0x67,0x43},
	// {0x68,0x54},
	// {0x69,0x65},
	// {0x6a,0x75},
	// {0x6b,0x93},
	// {0x6c,0xb0},
	// {0x6d,0xcb},
	// {0x6e,0xe6},
	// {0x6f,0xff},

    {0xFE , 0x01},  // page 1
    {0x18 , 0x02},
    {0xFE , 0x00},  // page 0
    {0x98 , 0x00},
    {0x9B , 0x20},
    {0x9C , 0x80},
    {0xA4 , 0x10},
    {0xA8 , 0xB0},
    {0xAA , 0x40},
    {0xA2 , 0x23},
    {0xAD , 0x01},

    {0xFE , 0x01},  // page 1
    {0x9C , 0x02},
    {0x08 , 0xA0},
    {0x09 , 0xE8},

    {0x10 , 0x00},
    {0x11 , 0x11},
    {0x12 , 0x10},
    {0x13 , 0x80},
    {0x15 , 0xFC},
    {0x18 , 0x03},
    {0x21 , 0xC0},
    {0x22 , 0x60},
    {0x23 , 0x30},
    {0x25 , 0x00},
    {0x24 , 0x14},

    {0xFE , 0x01},  // page 1
    {0xC0 , 0x10},
    {0xC1 , 0x0C},
    {0xC2 , 0x0A},
    {0xC6 , 0x0E},
    {0xC7 , 0x0B},
    {0xC8 , 0x0A},
    {0xBA , 0x26},
    {0xBB , 0x1C},
    {0xBC , 0x1D},
    {0xB4 , 0x23},
    {0xB5 , 0x1C},
    {0xB6 , 0x1A},
    {0xC3 , 0x00},
    {0xC4 , 0x00},
    {0xC5 , 0x00},
    {0xC9 , 0x00},
    {0xCA , 0x00},
    {0xCB , 0x00},
    {0xBD , 0x00},
    {0xBE , 0x00},
    {0xBF , 0x00},
    {0xB7 , 0x07},
    {0xB8 , 0x05},
    {0xB9 , 0x05},
    {0xA8 , 0x07},
    {0xA9 , 0x06},
    {0xAA , 0x00},
    {0xAB , 0x04},
    {0xAC , 0x00},
    {0xAD , 0x02},
    {0xAE , 0x0D},
    {0xAF , 0x05},
    {0xB0 , 0x00},
    {0xB1 , 0x07},
    {0xB2 , 0x03},
    {0xB3 , 0x00},
    {0xA4 , 0x00},
    {0xA5 , 0x00},
    {0xA6 , 0x00},
    {0xA7 , 0x00},
    {0xA1 , 0x3C},
    {0xA2 , 0x50},
    {0xFE , 0x00},  // page 0

    {0xB1 , 0x04},  
    {0xB2 , 0xFD},
    {0xB3 , 0xFC},
    {0xB4 , 0xF0},
    {0xB5 , 0x05},
    {0xB6 , 0xF0},

    // msleep(200);
    {0xFE , 0x00},  // page 0
    {0x27 , 0xF7},
    {0x28 , 0x7F},
    {0x29 , 0x20},
    {0x33 , 0x20},
    {0x34 , 0x20},
    {0x35 , 0x20},
    {0x36 , 0x20},
    {0x32 , 0x08},

    {0x47 , 0x00},
    {0x48 , 0x00},

    {0xFE , 0x01},  // page 1
    {0x79 , 0x00},  
    {0x7D , 0x00},
    {0x50 , 0x88},  // AWB_PRE_mode: [7] PRE_enable; [3] AWB_PRE_adjust_speed enable
    {0x5B , 0x0C},  
    {0x76 , 0x8F},
    {0x80 , 0x70},
    {0x81 , 0x70},
    {0x82 , 0xB0},
    {0x70 , 0xFF},
    {0x71 , 0x00},
    {0x72 , 0x28},
    {0x73 , 0x0B},
    {0x74 , 0x0B},

    {0xFE , 0x00},  // page 0
    {0x70 , 0x45},  // global gain
    {0x4F , 0x01},  // AEC enable

    // {0xF1 , 0x00},  // Pad_setting1: [2] pclk output enable; [1] HSYNC output enable; [0] VSYNC output enable
    // {0xF2 , 0x00},  // Pad_setting2: [0] data output enable
    {0xF1 , 0x00}, 
    {0xF2 , 0x01},
    {0 , 0}
};

// static const uint8_t qqvga_config[][2] = { //k210 
//     {0xfe , 0x00},
//     // window
//         //windowing mode
// 	{0x09 , 0x00},
//     {0x0a , 0x00},
// 	{0x0b , 0x00},
// 	{0x0c , 0x00},
//     {0x0d , 0x01},
// 	{0x0e , 0xe8},
// 	{0x0f , 0x02},
// 	{0x10 , 0x88},
//         //crop mode 
//     {0x50 , 0x01},
//     {0x51, 0x00},
//     {0x52, 0x00},
//     {0x53, 0x00},
//     {0x54, 0x00},
//     {0x55, 0x00},
//     {0x56, 0x78},
//     {0x57, 0x00},
//     {0x58, 0xA0},
//     //subsample 1/4
//     {0x59, 0x44},
//     {0x5a, 0x03},
//     {0x5b, 0x00},
//     {0x5c, 0x00},
//     {0x5d, 0x00},
//     {0x5e, 0x00},
//     {0x5f, 0x00},
//     {0x60, 0x00},
//     {0x61, 0x00},
//     {0x62, 0x00},

//     {0x00, 0x00}
// };

static const uint8_t qvga_config[][2] = { //k210 
    {0xfe , 0x00},
    // window
        //windowing mode
	{0x09 , 0x00},
    {0x0a , 0x00},
	{0x0b , 0x00},
	{0x0c , 0x00},
    {0x0d , 0x01},
	{0x0e , 0xe8},
	{0x0f , 0x02},
	{0x10 , 0x88},
        //crop mode 
    {0x50 , 0x01},
    // {0x51, 0x00},
    // {0x52, 0x78},
    // {0x53, 0x00},
    // {0x54, 0xa0},
    // {0x55, 0x00},
    // {0x56, 0xf0},
    // {0x57, 0x01},
    // {0x58, 0x40},
    //subsample 1/2
    {0x59, 0x22},
    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5c, 0x00},
    {0x5d, 0x00},
    {0x5e, 0x00},
    {0x5f, 0x00},
    {0x60, 0x00},
    {0x61, 0x00},
    {0x62, 0x00},

    {0x00, 0x00}
};

int sccb_i2c_write_byte( uint8_t addr, uint16_t reg, uint8_t reg_len, uint8_t data, uint16_t timeout_ms)
{
        uint8_t tmp[3];
        if (reg_len == 8)
        {
            tmp[0] = reg & 0xFF;
            tmp[1] = data;
            return maix_i2c_send_data(i2c_num, addr, tmp, 2, 10);
        }
        else
        {
            tmp[0] = (reg >> 8) & 0xFF;
            tmp[1] = reg & 0xFF;
            tmp[2] = data;
            return maix_i2c_send_data(i2c_num, addr, tmp, 3, 10);
        }
    return 0;
}

/**
 * @brief  
 * @note   
 * @param  addr: 
 * @param  reg: 
 * @param  reg_len: 
 * @param  *data: 
 * @param  timeout_ms: 
 * @retval 
 */
int sccb_i2c_read_byte( uint8_t addr, uint16_t reg, uint8_t reg_len, uint8_t *data, uint16_t timeout_ms)
{
    *data = 0;
        uint8_t tmp[2];
        if (reg_len == 8)
        {
            tmp[0] = reg & 0xFF;
            maix_i2c_send_data(i2c_num, addr, tmp, 1, 10);
            int ret = maix_i2c_recv_data(i2c_num, addr, NULL, 0, data, 1, 10);
            return ret;
        }
        else
        {
            tmp[0] = (reg >> 8) & 0xFF;
            tmp[1] = reg & 0xFF;
            int ret = maix_i2c_send_data(i2c_num, addr, tmp, 2, 10);
            if (ret != 0)
                return ret;
            ret = maix_i2c_recv_data(i2c_num,addr, NULL, 0, data, 1, 10);
            return ret;
        }
    
    return 0;
}

int sccb_i2c_recieve_byte( uint8_t addr, uint8_t *data, uint16_t timeout_ms)
{
        return maix_i2c_recv_data(i2c_num, addr, NULL, 0, data, 1, 10);
}


int cambus_readb(uint8_t slv_addr, uint16_t reg_addr, uint8_t *reg_data)
{

    int ret = 0;
    sccb_i2c_read_byte(slv_addr, reg_addr, sccb_reg_width, reg_data, 10);
    if (0xff == *reg_data)
        ret = -1;

    return ret;
}

int cambus_writeb(uint8_t slv_addr, uint16_t reg_addr, uint8_t reg_data)
{
    int ret = sccb_i2c_write_byte(slv_addr, reg_addr, sccb_reg_width, reg_data, 10);
	   if (0 != ret)
	   		printf("cambus_writeb ret:%d\n",ret);
    msleep(2);
    return 0;
}

int gc0328_read_reg(uint8_t reg_addr)
{
    uint8_t reg_data;
    if (cambus_readb(GC0328_ADDR, reg_addr, &reg_data) != 0) {
        return -1;
    }
    return reg_data;
}

int gc0328_write_reg(uint8_t reg_addr, uint16_t reg_data)
{
    return cambus_writeb(GC0328_ADDR, reg_addr, (uint8_t)reg_data);
}


int gc0328_set_framesize()
{
    int ret=0;
    int i=0;

    while (qvga_config[i][0]) {
        cambus_writeb(GC0328_ADDR, qvga_config[i][0], qvga_config[i][1]);
        i++;
    }
    /* delay n ms */
    msleep(30);
	dvp_set_image_size(320, 240);
    return ret;
}

#define NUM_CONTRAST_LEVELS (5)
static uint8_t contrast_regs[NUM_CONTRAST_LEVELS][2]={
	{0x80, 0x00},
	{0x80, 0x20},
	{0x80, 0x40},
	{0x80, 0x60},
	{0x80, 0x80}
};

int gc0328_set_contrast(int level)
{
    int ret=0;

    level += (NUM_CONTRAST_LEVELS / 2);
    if (level < 0 || level > NUM_CONTRAST_LEVELS) {
        return -1;
    }
	cambus_writeb(GC0328_ADDR, 0xfe, 0x00);
	cambus_writeb(GC0328_ADDR, 0xd4, contrast_regs[level][0]);
	cambus_writeb(GC0328_ADDR, 0xd3, contrast_regs[level][1]);
    return ret;
}

int gc0328_set_brightness(int level)
{
    int ret = 0;
    return ret;
}

#define NUM_SATURATION_LEVELS (5)
static uint8_t saturation_regs[NUM_SATURATION_LEVELS][3]={
	{0x00, 0x00, 0x00},
	{0x10, 0x10, 0x10},
	{0x20, 0x20, 0x20},
	{0x30, 0x30, 0x30},
	{0x40, 0x40, 0x40},
};
int gc0328_set_saturation(int level)
{
    int ret = 0;
    level += (NUM_CONTRAST_LEVELS / 2);
    if (level < 0 || level > NUM_CONTRAST_LEVELS) {
        return -1;
    }
	cambus_writeb(GC0328_ADDR, 0xfe, 0x00);
	cambus_writeb(GC0328_ADDR, 0xd0, saturation_regs[level][0]);
	cambus_writeb(GC0328_ADDR, 0xd1, saturation_regs[level][1]);
	cambus_writeb(GC0328_ADDR, 0xd2, saturation_regs[level][2]);
    return ret;
}

// int gc0328_set_gainceiling(gainceiling_t gainceiling)
// {
//     int ret = 0;

//     return ret;
// }

int gc0328_set_quality( int qs)
{
    int ret = 0;

    return ret;
}

int gc0328_set_colorbar( int enable)
{
    int ret = 0;
    return ret;
}

int gc0328_set_auto_gain( int enable, float gain_db, float gain_db_ceiling)
{
   int ret = 0;
   return ret;
}

int gc0328_get_gain_db( float *gain_db)
{
    int ret = 0;
    return ret;
}

int gc0328_set_auto_exposure( int enable, int exposure_us)
{
    int ret = 0;
	uint8_t temp;
	cambus_writeb(GC0328_ADDR, 0xfe, 0x00);
	cambus_readb(GC0328_ADDR, 0x4f, &temp);
	if(enable != 0)
	{
		cambus_writeb(GC0328_ADDR,0x4f, temp|0x01); // enable
		if(exposure_us != -1)
		{
			cambus_writeb(GC0328_ADDR, 0xfe, 0x01);
			cambus_writeb(GC0328_ADDR,0x2f, (uint8_t)(((uint16_t)exposure_us)>>8));
			cambus_writeb(GC0328_ADDR,0x30, (uint8_t)(((uint16_t)exposure_us)));
		}
	}
	else
	{
		cambus_writeb(GC0328_ADDR,0x4f, temp&0xfe); // disable
		if(exposure_us != -1)
		{
			cambus_writeb(GC0328_ADDR, 0xfe, 0x01);
			cambus_writeb(GC0328_ADDR,0x2f, (uint8_t)(((uint16_t)exposure_us)>>8));
			cambus_writeb(GC0328_ADDR,0x30, (uint8_t)(((uint16_t)exposure_us)));
		}
	}
    return ret;
}

int gc0328_get_exposure_us(int *exposure_us)
{
    int ret = 0;
    return ret;
}

// int gc0328_set_auto_whitebal( int enable, float r_gain_db, float g_gain_db, float b_gain_db)
// {
//     int ret = 0;
// 	uint8_t temp;
// 	cambus_writeb(GC0328_ADDR, 0xfe, 0x00);
// 	cambus_readb(GC0328_ADDR, 0x42, &temp);
// 	if(enable != 0)
// 	{
// 		cambus_writeb(GC0328_ADDR,0x42, temp|0x02); // enable
// 		if(!isnanf(r_gain_db))
// 			cambus_writeb(GC0328_ADDR,0x80, (uint8_t)r_gain_db); //limit
// 		if(!isnanf(g_gain_db))
// 			cambus_writeb(GC0328_ADDR,0x81, (uint8_t)g_gain_db);
// 		if(!isnanf(b_gain_db))
// 			cambus_writeb(GC0328_ADDR,0x82, (uint8_t)b_gain_db);
// 	}
// 	else
// 	{
// 		cambus_writeb(GC0328_ADDR,0x42, temp&0xfd); // disable
// 		if(!isnanf(r_gain_db))
// 			cambus_writeb(GC0328_ADDR,0x77, (uint8_t)r_gain_db);
// 		if(!isnanf(g_gain_db))
// 			cambus_writeb(GC0328_ADDR,0x78, (uint8_t)g_gain_db);
// 		if(!isnanf(b_gain_db))
// 			cambus_writeb(GC0328_ADDR,0x79, (uint8_t)b_gain_db);
// 	}
//     return ret;
// }

int gc0328_get_rgb_gain_db( float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    int ret = 0;
    return ret;
}

int gc0328_set_hmirror(int vflip, int enable)
{
    uint8_t data;
    cambus_readb(GC0328_ADDR, 0x17, &data);
	data = data & 0xFC;
    if(enable){
        data = data | 0x01 | (vflip ? 0x02 : 0x00);
    }else{
        data = data | (vflip ? 0x02 : 0x00);
    }
    return cambus_writeb(GC0328_ADDR, 0x17, data);
}

int gc0328_set_vflip(int hmirror, int enable)
{
    uint8_t data;
    cambus_readb(GC0328_ADDR, 0x17, &data);
	data = data & 0xFC;
    if(enable){
        data = data | 0x02 | (hmirror ? 0x01 : 0x00);
    }else{
        data = data | (hmirror ? 0x01 : 0x00);
    }
    return cambus_writeb(GC0328_ADDR, 0x17, data);
}

/**
 * @brief  初始化RGB摄像头参数
 * @note   
 * @retval 
 */
int gc0328_init0(void)
{
    uint16_t index = 0;
    
    cambus_writeb(GC0328_ADDR, 0xfe, 0x01);
    for (index = 0; sensor_rgb_regs[index][0]; index++)
    {
        if(sensor_rgb_regs[index][0] == 0xff){
            msleep(sensor_rgb_regs[index][1]);
            continue;
        }
        // printf("0x21,0x%02x,0x%02x,\r\n", sensor_default_regs[index][0], sensor_default_regs[index][1]);//debug
        cambus_writeb(GC0328_ADDR, sensor_rgb_regs[index][0], sensor_rgb_regs[index][1]);
        // msleep(1);
    }
    return 0;
}

/**
 * @brief  初始化红外摄像头参数
 * @note   
 * @retval 
 */
int gc0328_init1(void)
{
    uint16_t index = 0;
    
    cambus_writeb(GC0328_ADDR, 0xfe, 0x01);
    for (index = 0; sensor_ir_regs[index][0]; index++)
    {
        if(sensor_ir_regs[index][0] == 0xff){
            msleep(sensor_ir_regs[index][1]);
            continue;
        }
        // mp_printf(&mp_plat_print, "0x12,0x%02x,0x%02x,\r\n", sensor_default_regs[index][0], sensor_default_regs[index][1]);//debug
        cambus_writeb(GC0328_ADDR, sensor_ir_regs[index][0], sensor_ir_regs[index][1]);
        // msleep(1);
    }
    return 0;
}

int gc0328_read_id(void)
{
    uint8_t id;
    DCMI_RESET_LOW();    //reset gc0328
    msleep(10);
    DCMI_RESET_HIGH();
    msleep(10);

    int init_ret = 0;
    /* Reset the sensor */
    DCMI_RESET_HIGH();
    msleep(10);
    DCMI_RESET_LOW();
    msleep(30);
    sccb_reg_width = 8;
    sccb_i2c_write_byte( GC0328_ADDR, 0xFE, sccb_reg_width, 0x00, 10);
    sccb_i2c_read_byte( GC0328_ADDR, 0xF0, sccb_reg_width, &id, 10);
    if (id != 0x9d)
    {
        printf( "error gc0328 detect, ret id is 0x%x\r\n", id);
        return 0;
    }
    return id;
}

/**
 * @brief  设置i2c接口
 * @note   使用同一个I2C接口，根据num切换不同摄像头编号
 * @param  num: 摄像头编号
 * @retval None
 */
void i2c_master_init(int num)
{
	if(num)
	{
		fpioa_set_function(41, FUNC_I2C0_SCLK);//i2c_pin_clk
		fpioa_set_function(42, FUNC_RESV0);
		fpioa_set_function(40, FUNC_I2C0_SDA);
	}
	else
	{
		fpioa_set_function(41, FUNC_I2C0_SCLK);//i2c_pin_clk
		fpioa_set_function(40, FUNC_RESV0);
		fpioa_set_function(42, FUNC_I2C0_SDA);
	
	}
	maix_i2c_init(i2c_num, 7, 100000);//i2c addr= 0
}
/**
 * @brief  rgb摄像头
 * @note   
 * @retval None
 */
void open_gc0328_0()
{
	usleep(1000);
	i2c_master_init(1);
	gc0328_write_reg(0xFE,0x00);
	gc0328_write_reg(0xF1,0x00);
	gc0328_write_reg(0xF2,0x00);
	usleep(1000);
	i2c_master_init(0);
	gc0328_write_reg(0xFE,0x00);
	gc0328_write_reg(0xF1,0x07);
	gc0328_write_reg(0xF2,0x01);
}
/**
 * @brief  IR摄像头
 * @note   
 * @retval None
 */
void open_gc0328_1()
{
	i2c_master_init(0);
	gc0328_write_reg(0xFE,0x00);
	gc0328_write_reg(0xF1,0x00);
	gc0328_write_reg(0xF2,0x00);
	usleep(1000);
	i2c_master_init(1);
	gc0328_write_reg(0xFE,0x00);
	gc0328_write_reg(0xF1,0x07);
	gc0328_write_reg(0xF2,0x01);
}

int gc0328_init()
{
	printf("gc0328 init start\n");
	//初始化摄像头引脚
	fpioa_set_function(47, FUNC_CMOS_PCLK);//cmos_pclk
    fpioa_set_function(46, FUNC_CMOS_XCLK);//cmos_xclk
    fpioa_set_function(45, FUNC_CMOS_HREF);//cmos_href
    fpioa_set_function(44, FUNC_CMOS_PWDN);//cmos_pwdn
    fpioa_set_function(43, FUNC_CMOS_VSYNC);//cmos_vsync
    fpioa_set_function(17, FUNC_CMOS_RST);     //cmos_rst
	//初始化RGB摄像头
	i2c_master_init(0);
	//读取gc0328地址
	DCMI_PWDN_LOW(); //enable gc0328 要恢复 normal 工作模式，需将 PWDN pin 接入低电平即可，同时写入初始化寄存器即可
	uint16_t id = gc0328_read_id();
	if (id == 0)
    {
                    //should do something?
		 printf("gc0328 read id:%d\r\n",id);
       	 return -2;
    }
	printf( "[F1]: rgb sensor id = %x\r\n", id);
	    //select first sensor ,  Call sensor-specific reset function
    // DCMI_RESET_LOW();
    // msleep(10);
    // DCMI_RESET_HIGH();
    // msleep(10);
	gc0328_init0();
	gc0328_set_framesize();
	usleep(1000);
	#if 1
	//初始化红外摄像头
	i2c_master_init(1);
	id = gc0328_read_id();
	if (id == 0)
    {
                    //should do something?
       	 return -2;
    }
	printf( "[F1]: IR sensor id = %x\r\n", id);
	// DCMI_RESET_LOW();
    // msleep(10);
    // DCMI_RESET_HIGH();
    // msleep(10);
	gc0328_init1();
	gc0328_set_framesize();
	#endif
}
// int gc0328_set_windowing(framesize_t framesize, int x, int y, int w, int h)
// {
// 	if(framesize == FRAMESIZE_QVGA)
// 	{
// 		cambus_writeb(GC0328_ADDR, 0xfe, 0x00);
// 		cambus_writeb(GC0328_ADDR, 0x51, (y>>8) & 0xff);
// 		cambus_writeb(GC0328_ADDR, 0x52, y & 0xff);
// 		cambus_writeb(GC0328_ADDR, 0x53, (x>>8) & 0xff);
// 		cambus_writeb(GC0328_ADDR, 0x54, x & 0xff);
// 		cambus_writeb(GC0328_ADDR, 0x55, (h>>8) & 0x01);
// 		cambus_writeb(GC0328_ADDR, 0x56, h & 0xff);
// 		cambus_writeb(GC0328_ADDR, 0x57, (w>>8) & 0x03);
// 		cambus_writeb(GC0328_ADDR, 0x58, w & 0xff);
// 	}
// 	return 0;
// }



