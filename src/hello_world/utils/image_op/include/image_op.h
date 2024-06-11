#ifndef __IMAGE_OP_H
#define __IMAGE_OP_H

#include <stdint.h>

void image_rgb888_roate_right90(uint8_t *out, uint8_t *src, uint16_t w,
                                uint16_t h);
void image_rgb888_roate_left90(uint8_t *out, uint8_t *src, uint16_t w,
                               uint16_t h);

void convert_rgb565_order(uint16_t *image, uint16_t w, uint16_t h);

void image_rgb565_roate_right90_lessmem(uint16_t *buf, uint16_t w, uint16_t h);
void image_rgb565_roate_right90(uint16_t *out, uint16_t *src, uint16_t w,
                                uint16_t h);
void image_rgb565_roate_left90(uint16_t *out, uint16_t *src, uint16_t w,
                               uint16_t h);

void image_r8g8b8_roate_right90(uint8_t *out, uint8_t *src, uint16_t w,
                                uint16_t h);
void image_r8g8b8_roate_left90(uint8_t *out, uint8_t *src, uint16_t w,
                               uint16_t h);

void image_rgb5652rgb888(uint16_t *rgb565, uint8_t *rgb888, uint16_t img_w,
                         uint16_t img_h);

void image_rgb565_draw_edge(uint32_t *gram,
                            uint16_t x1, uint16_t y1,
                            uint16_t x2, uint16_t y2,
                            uint16_t color, uint16_t img_w, uint16_t img_h);

void image_rgb565_draw_string(uint16_t *ptr, char *str, uint8_t size,
                              uint16_t x, uint16_t y,
                              uint16_t color, uint16_t *bg_color,
                              uint16_t img_w, uint16_t img_h);

typedef uint8_t (*get_zhCN_dat)(uint8_t *zhCN_char, uint8_t *zhCN_dat, uint8_t size);

void image_rgb565_draw_zhCN_string(uint16_t *ptr, uint8_t *zhCN_string, uint8_t size,
                                   uint16_t x, uint16_t y,
                                   uint16_t color, uint16_t *bg_color,
                                   uint16_t img_w, uint16_t img_h,
                                   get_zhCN_dat get_font_data);

typedef struct
{
  uint16_t *img_addr;
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
} mix_image_t;

void image_rgb565_mix_pic_with_alpha_lessmem(mix_image_t *img_src, mix_image_t *img_dst,
                                             uint32_t dst_addr, uint32_t alpha, uint8_t del_white,
                                             int (*flash_read_data)(uint32_t addr, uint8_t *data_buf, uint32_t length));

void image_rgb565_mix_pic_with_alpha(mix_image_t *img_src, mix_image_t *img_dst, uint32_t alpha, uint8_t del_white);

#endif
