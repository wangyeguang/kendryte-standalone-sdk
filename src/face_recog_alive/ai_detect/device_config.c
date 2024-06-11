#include "device_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "sysctl.h"

#define		OTP_MANUFACTURE_DATA_ADDR		0x3D90
/* clang-format off */
#define DELAY_TIMEOUT       0xFFFFFFU
#define WRTEST_NUM          0xA5U
/* clang-format on */
#define OTP_BISR_DATA_ADDR      0x00003DD0U


typedef struct _otp
{
    volatile uint32_t otp_ceb;
    volatile uint32_t otp_test_mode;
    volatile uint32_t otp_mode;
    volatile uint32_t gb_otp_en;
    volatile uint32_t dat_in_finish;
    volatile uint32_t otp_bisr_fail;
    volatile uint32_t test_step;
    volatile uint32_t otp_pwrrdy;
    volatile uint32_t otp_last_dat;
    volatile uint32_t otp_data;
    volatile uint32_t otp_pwr_mode;
    volatile uint32_t otp_in_dat;
    volatile uint32_t otp_apb_adr;
    volatile uint32_t td_result;
    volatile uint32_t data_acp_flag;
    volatile uint32_t otp_adr_in_flag;
    volatile uint32_t wr_result;
    volatile uint32_t otp_thershold;
    volatile uint32_t bisr_finish;
    volatile uint32_t key_cmp_result;
    volatile uint32_t otp_cmp_key;
    volatile uint32_t cmp_result_rdy;
    volatile uint32_t otp_cle;
    volatile uint32_t data_blk_ctrl;
    volatile uint32_t otp_wrg_adr_flag;
    volatile uint32_t pro_wrong;
    volatile uint32_t otp_status;
    volatile uint32_t otp_pro_adr;
    volatile uint32_t blank_finish;
    volatile uint32_t bisr2otp_en;
    volatile uint32_t otp_cpu_ctrl;
    volatile uint32_t otp_web_cpu;
    volatile uint32_t otp_rstb_cpu;
    volatile uint32_t otp_seltm_cpu;
    volatile uint32_t otp_readen_cpu;
    volatile uint32_t otp_pgmen_cpu;
    volatile uint32_t otp_dle_cpu;
    volatile uint32_t otp_din_cpu;
    volatile uint32_t otp_cpumpen_cpu;
    volatile uint32_t otp_cle_cpu;
    volatile uint32_t otp_ceb_cpu;
    volatile uint32_t otp_adr_cpu;
    volatile uint32_t otp_dat_cpu;
    volatile uint32_t otp_data_rdy;
    volatile uint32_t block_flag_high;
    volatile uint32_t block_flag_low;
    volatile uint32_t reg_flag_high;
    volatile uint32_t reg_flag_low;
} __attribute__((packed, aligned(4))) my_otp_t;

volatile my_otp_t* const my_otp = (volatile my_otp_t*)OTP_BASE_ADDR;
/* Manufacture data (Start address 0x3D90) */
typedef struct {

	uint8_t 	cp_kgd;
	uint8_t 	ft_kgd;
	uint8_t		work_order_id[10];
    union {
        uint32_t time;
        struct
        {
            uint32_t second: 6;
            uint32_t minute: 6;
            uint32_t hour: 	 5;
			uint32_t day:    5;
            uint32_t month:  4;
            uint32_t year:   6;
        }data;
    }id1 ;
    union {
        uint16_t no;
        struct
        {
            uint16_t site_no: 6;
			uint16_t ate_no:  10;
        } data;
    }id2;	
    union {
        uint8_t sit;
        struct
        {
            uint8_t package_site: 5;
			uint8_t	fab_site:	  3;
        } data;
    } site;
    union {
        uint32_t version;
        struct
        {
            uint32_t chip_rev: 	4;
			uint32_t chip_code: 10;
			uint32_t chip_no: 	10;
			uint32_t chip_name:	8;	
        } data;
    } version;
	uint8_t diey;
	uint8_t diex;
	uint8_t wafer_id;
	uint8_t lot_id[6];
}__attribute__((packed, aligned(4))) manufacture_data_t;


static int myotp_read_byte(uint32_t addr, uint8_t* data_buf, uint32_t length)
{
    uint32_t time_out;

    my_otp->otp_cle = 0;
    my_otp->otp_mode = 0;
    my_otp->otp_wrg_adr_flag = 0;
    my_otp->otp_ceb = 0;
    addr *= 8;
    while (length--)
    {
        time_out = 0;
        while (my_otp->otp_adr_in_flag == 0)
        {
            time_out++;
            if (time_out >= DELAY_TIMEOUT)
                return 1;
        }
        if (length == 0)
            my_otp->otp_last_dat = 1;
        my_otp->otp_apb_adr = addr;
        time_out = 0;
        while (my_otp->otp_data_rdy == 0)
        {
            time_out++;
            if (time_out >= DELAY_TIMEOUT)
                return 1;
        }
        if (my_otp->otp_wrg_adr_flag == 0x01)
            return 2;
        *data_buf++ = my_otp->otp_data;
        addr += 8;
    }
    return 0;
}

int myotp_read_data(uint32_t addr, uint8_t* data_buf, uint32_t length)
{
    int status;

    if (addr >= OTP_BISR_DATA_ADDR)
        return 2;
    length = length <= OTP_BISR_DATA_ADDR - addr ? length : OTP_BISR_DATA_ADDR - addr;

    status = myotp_read_byte(addr, data_buf, length);
    if (status == 2)
        status = 10;
    return status;
}

uint64_t swap_endian(uint64_t value)
{
return ((value & 0xFF0000000000) >> 40) |
           ((value & 0x00FF00000000) >> 24) |
           ((value & 0x0000FF000000) >> 8) |
           ((value & 0x000000FF0000) << 8) |
           ((value & 0x00000000FF00) << 24) |
           ((value & 0x0000000000FF) << 40);

}
uint64_t get_chip_number(void)
{
    uint64_t devid = 0;
    manufacture_data_t *manufacture_data = malloc(sizeof(manufacture_data_t));
    if(0 == myotp_read_data(OTP_MANUFACTURE_DATA_ADDR, (uint8_t *)manufacture_data, sizeof(manufacture_data_t)) ) {
		
		devid = manufacture_data->id1.time;
		devid <<= 16;
		devid |= manufacture_data->id2.no;
		
        devid = devid & 0x0000FFFFFFFFFFFF;
        devid = swap_endian(devid);
        printf("devid= %lx\n", devid); //maixpy读取为逆序

	}
	else {
		printf("get device number error !\n");
	}	
	
	free(manufacture_data);
    return devid;
}

uint64_t get_time_ms(void)
{
    return sysctl_get_time_us()/1000;
}