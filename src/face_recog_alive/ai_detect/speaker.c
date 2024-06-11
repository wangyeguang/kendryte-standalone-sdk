#include "speaker.h"

#include <stdio.h>
#include <sysctl.h>
#include <fpioa.h>
#include <gpio.h>
#include <i2s.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "w25qxx.h"
#include "device_config.h"

#define DMA_IRQ 1

#define FRAME_LEN   128
uint32_t rx_buf[1024];

void speaker_init()
{
    fpioa_set_function(21, FUNC_I2S2_OUT_D1);
    fpioa_set_function(23, FUNC_I2S2_SCLK);
    fpioa_set_function(22, FUNC_I2S2_WS);
    fpioa_set_function(18, FUNC_GPIO3);//spk en

    gpio_set_drive_mode(3,GPIO_DM_OUTPUT);
    gpio_pin_value_t value = GPIO_PV_LOW;
    gpio_set_pin(3, value); // low is off , high is on
    i2s_init(I2S_DEVICE_2, I2S_TRANSMITTER, 0xC); //channel_num= 1 , 0x03 << channel_num *2
    i2s_tx_channel_config(I2S_DEVICE_2, I2S_CHANNEL_1,
        RESOLUTION_16_BIT, SCLK_CYCLES_32,
        TRIGGER_LEVEL_4,
        RIGHT_JUSTIFYING_MODE
        );
    i2s_set_sample_rate(I2S_DEVICE_2,(16000));
}

void speaker_play(uint8_t *audio_buf, uint32_t  audio_len)
{
    // i2s_data_t data = (i2s_data_t)
    // {
    //     .tx_channel = DMAC_CHANNEL1,
    //     .tx_buf = buf,
    //     .tx_len = len, 
    //     .transfer_mode = I2S_SEND,
    // };
    // i2s_handle_data_dma(I2S_DEVICE_2,data,NULL);
    gpio_set_pin(3, GPIO_PV_HIGH); 
    i2s_play(I2S_DEVICE_2, I2S_CHANNEL_1, audio_buf, audio_len, 512, 16, 1);
    msleep(10);
    gpio_set_pin(3, GPIO_PV_LOW); 
}

void speaker_stop()
{
}

void speaker_pause()
{

}

void speaker_resume()
{

}

void speaker_set_volume(int volume)
{

}

void speaker_set_speed(int speed)
{

}

void speaker_play_welcome()
{
    uint8_t *wav_data = malloc((AUDIO_WELCOM_SIZE));
    uint32_t wav_size = 34 * 512; //17920;
     if (wav_data)
    {
        w25qxx_read_data(AUDIO_WELCOM_ADDR, wav_data, AUDIO_WELCOM_SIZE,W25QXX_QUAD_FAST);
        speaker_play(wav_data + (AUDIO_WELCOM_SIZE - wav_size - 2048), wav_size + 256);
        free(wav_data);
    }
}

void speaker_play_lock()
{
        uint8_t *wav_data = malloc((AUDIO_UNLOCK_SIZE));
    uint32_t wav_size = 34 * 512; //17920;
     if (wav_data)
    {
        w25qxx_read_data(AUDIO_UNLOCK_ADDR, wav_data, AUDIO_UNLOCK_SIZE,W25QXX_QUAD_FAST);
        speaker_play(wav_data + (AUDIO_UNLOCK_SIZE - wav_size - 2048), wav_size + 256);
        free(wav_data);
    }
}

void speaker_play_unlock()
{
    uint8_t *wav_data = malloc((AUDIO_LOCK_SIZE));
    uint32_t wav_size = 34 * 512; //17920;
     if (wav_data)
    {
        w25qxx_read_data(AUDIO_LOCK_ADDR, wav_data, AUDIO_LOCK_SIZE,W25QXX_QUAD_FAST);
        speaker_play(wav_data + (AUDIO_LOCK_SIZE - wav_size - 2048), wav_size + 256);
        free(wav_data);
    }
}