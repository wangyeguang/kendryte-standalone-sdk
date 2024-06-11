#include "image_op.h"

#include "ascii_font.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// RGB888 顺时针 90 度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb888_roate_right90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for (uint32_t i = 0; i < w; i++)
    {
        for (uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0)) * 3 + 0) = *(src + ((h - 1 - (j * 2 + 0)) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 0)) * 3 + 1) = *(src + ((h - 1 - (j * 2 + 0)) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 0)) * 3 + 2) = *(src + ((h - 1 - (j * 2 + 0)) * w + i) * 3 + 2);

            *(out + (i * h + (j * 2 + 1)) * 3 + 0) = *(src + ((h - 1 - (j * 2 + 1)) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 1)) * 3 + 1) = *(src + ((h - 1 - (j * 2 + 1)) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 1)) * 3 + 2) = *(src + ((h - 1 - (j * 2 + 1)) * w + i) * 3 + 2);
        }
    }
    return;
}

// RGB888 逆时针90度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb888_roate_left90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for (uint32_t i = 0; i < w; i++)
    {
        for (uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0)) * 3 + 0) = *(src + ((j * 2 + 0) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 0)) * 3 + 1) = *(src + ((j * 2 + 0) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 0)) * 3 + 2) = *(src + ((j * 2 + 0) * w + i) * 3 + 2);

            *(out + (i * h + (j * 2 + 1)) * 3 + 0) = *(src + ((j * 2 + 1) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 1)) * 3 + 1) = *(src + ((j * 2 + 1) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 1)) * 3 + 2) = *(src + ((j * 2 + 1) * w + i) * 3 + 2);
        }
    }
    return;
}

void convert_rgb565_order(uint16_t *image, uint16_t w, uint16_t h)
{
    uint16_t tmp;

    for (uint16_t i = 0; i < h; i++)
    {
        for (uint16_t j = 0; j < (w / 2); j++)
        {
            tmp = *(image + i * w + j * 2 + 0);
            *(image + i * w + j * 2 + 0) = *(image + i * w + j * 2 + 1);
            *(image + i * w + j * 2 + 1) = tmp;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
#define LOOP_UNIT()                         \
    {                                       \
        next = h - 1 - i / w + (i % w) * h; \
        tmp = buf[next];                    \
        buf[next] = t;                      \
        t = tmp;                            \
        i = next;                           \
    }

#define I_UNIT(x)    \
    i = x;           \
    cycleBegin = i;  \
    t = buf[i];      \
    do               \
    {                \
        LOOP_UNIT(); \
        LOOP_UNIT(); \
    } while (i != cycleBegin);

void image_rgb565_roate_right90_lessmem(uint16_t *buf, uint16_t w, uint16_t h)
{
    uint16_t t; //暂存一个交换像素
    uint16_t tmp;
    int next;       //下一个像素的地址
    int cycleBegin; //本轮起始地址
    int i;          //计数器

    I_UNIT(0);
    I_UNIT(10);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//FIXME: 需要调用 convert_rgb565_order
// RGB565 顺时针 90 度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb565_roate_right90(uint16_t *out, uint16_t *src, uint16_t w, uint16_t h)
{
    for (uint32_t i = 0; i < w; i++)
    {
        for (uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((h - 1 - (j * 2 + 0)) * w + i));

            *(out + (i * h + (j * 2 + 1))) = *(src + ((h - 1 - (j * 2 + 1)) * w + i));
        }
    }
    return;
}

//FIXME: 需要调用 convert_rgb565_order
// RGB565 逆时针90度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb565_roate_left90(uint16_t *out, uint16_t *src, uint16_t w, uint16_t h)
{
    for (uint32_t i = 0; i < w; i++)
    {
        for (uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((j * 2 + 0) * w + i));

            *(out + (i * h + (j * 2 + 1))) = *(src + ((j * 2 + 1) * w + i));
        }
    }
    return;
}

// R8G8B8 顺时针 90 度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_r8g8b8_roate_right90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for (uint32_t i = 0; i < w; i++)
    {
        for (uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((h - 1 - (j * 2 + 0)) * w + i));
            *(out + (i * h + (j * 2 + 1))) = *(src + ((h - 1 - (j * 2 + 1)) * w + i));
        }
    }
    return;
}

// R8G8B8 逆时针90度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_r8g8b8_roate_left90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for (uint32_t i = 0; i < w; i++)
    {
        for (uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((j * 2 + 0) * w + i));
            *(out + (i * h + (j * 2 + 1))) = *(src + ((j * 2 + 1) * w + i));
        }
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* clang-format off */
#define RGB565_RED          (0xf800)
#define RGB565_GREEN        (0x07e0)
#define RGB565_BLUE         (0x001f)
/* clang-format on */

void image_rgb5652rgb888(uint16_t *rgb565, uint8_t *rgb888,
                         uint16_t img_w, uint16_t img_h)
{
    uint16_t pixel = 0;

    for (uint16_t i = 0; i < img_h; i++)
    {
        for (uint16_t j = 0; j < img_w / 2; j++)
        {
            pixel = (uint16_t) * (rgb565 + i * img_w + (j * 2 + 1));
            *(rgb888 + ((j * 2) + i * img_w) * 3 + 0) = (pixel & RGB565_RED) >> 8;
            *(rgb888 + ((j * 2) + i * img_w) * 3 + 1) = (pixel & RGB565_GREEN) >> 3;
            *(rgb888 + ((j * 2) + i * img_w) * 3 + 2) = (pixel & RGB565_BLUE) << 3;

            pixel = (uint16_t) * (rgb565 + i * img_w + (j * 2));
            *(rgb888 + ((j * 2 + 1) + i * img_w) * 3 + 0) = (pixel & RGB565_RED) >> 8;
            *(rgb888 + ((j * 2 + 1) + i * img_w) * 3 + 1) = (pixel & RGB565_GREEN) >> 3;
            *(rgb888 + ((j * 2 + 1) + i * img_w) * 3 + 2) = (pixel & RGB565_BLUE) << 3;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void image_rgb565_draw_edge(uint32_t *gram,
                            uint16_t x1, uint16_t y1,
                            uint16_t x2, uint16_t y2,
                            uint16_t color, uint16_t img_w, uint16_t img_h)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4;

    if (x1 <= 0)
        x1 = 1;
    if (x2 >= img_w - 1)
        x2 = img_w - 2;
    if (y1 <= 0)
        y1 = 1;
    if (y2 >= img_h - 1)
        y2 = img_h - 2;

    addr1 = gram + (img_w * y1 + x1) / 2;
    addr2 = gram + (img_w * y1 + x2 - 8) / 2;
    addr3 = gram + (img_w * (y2 - 1) + x1) / 2;
    addr4 = gram + (img_w * (y2 - 1) + x2 - 8) / 2;

    for (uint32_t i = 0; i < 4; i++)
    {
        *addr1 = data;
        *(addr1 + img_w / 2) = data;
        *addr2 = data;
        *(addr2 + img_w / 2) = data;
        *addr3 = data;
        *(addr3 + img_w / 2) = data;
        *addr4 = data;
        *(addr4 + img_w / 2) = data;
        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }

    addr1 = gram + (img_w * y1 + x1) / 2;
    addr2 = gram + (img_w * y1 + x2 - 2) / 2;
    addr3 = gram + (img_w * (y2 - 8) + x1) / 2;
    addr4 = gram + (img_w * (y2 - 8) + x2 - 2) / 2;

    for (uint32_t i = 0; i < 8; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;
        addr1 += img_w / 2;
        addr2 += img_w / 2;
        addr3 += img_w / 2;
        addr4 += img_w / 2;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//取模方式： 阴码，逐列式，顺向
static void image_rgb565_ram_draw_font_mat(uint16_t *ptr, uint8_t zhCN,
                                           uint8_t *font_mat, uint8_t size,
                                           uint16_t x, uint16_t y,
                                           uint16_t color, uint16_t *bg_color,
                                           uint16_t img_w)
{
    uint8_t data, csize;
    uint16_t *addr = NULL;
    uint16_t oy = y;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (zhCN ? size : (size / 2));

    for (uint8_t i = 0; i < csize; i++)
    {
        data = *(font_mat + i);
        for (uint8_t j = 0; j < 8; j++)
        {
            addr = ((uint16_t *)ptr) + y * img_w + x;

            if (data & 0x80)
            {
                if (x & 1)
                    *(addr - 1) = color;
                else
                    *(addr + 1) = color;
            }
            else
            {
                if (bg_color)
                {
                    if (x & 1)
                        *(addr - 1) = *bg_color;
                    else
                        *(addr + 1) = *bg_color;
                }
            }
            data <<= 1;
            y++;
            if ((y - oy) == size)
            {
                y = oy;
                x++;
                break;
            }
        }
    }
    return;
}

static void image_rgb565_ram_draw_char(uint16_t *ptr, char c, uint8_t size,
                                       uint16_t x, uint16_t y,
                                       uint16_t color, uint16_t *bg_color,
                                       uint16_t img_w)
{
    if (c < ' ' || c > '~')
    {
        return;
    }

    uint8_t offset = c - ' ';

    switch (size)
    {
    case 16:
        image_rgb565_ram_draw_font_mat(ptr, 0, ascii0816 + (offset * 16), size, x, y, color, bg_color, img_w);
        break;
    case 32:
        image_rgb565_ram_draw_font_mat(ptr, 0, ascii_1632 + (offset * 64), size, x, y, color, bg_color, img_w);
        break;
    default:
        break;
    }
    return;
}

void image_rgb565_draw_string(uint16_t *ptr, char *str, uint8_t size,
                              uint16_t x, uint16_t y,
                              uint16_t color, uint16_t *bg_color,
                              uint16_t img_w, uint16_t img_h)
{
    uint16_t ox = x;
    uint16_t oy = y;

    if (size != 32 && size != 16)
    {
        return;
    }

    while (*str)
    {
        if (x > (ox + img_w - size / 2))
        {
            y += size;
            x = ox;
        }
        if (y > (oy + img_h - size))
            break;

        if ((*str == 0xd) ||
            (*str == 0xa))
        {
            y += size;
            x = ox;
            str++;
        }
        else
        {
            image_rgb565_ram_draw_char(ptr, *str, size, x, y, color, bg_color, img_w);
        }
        str++;
        x += size / 2;
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void image_rgb565_draw_zhCN_char(uint16_t *ptr, uint8_t *zhCN_char, uint8_t size,
                                        uint16_t x, uint16_t y,
                                        uint16_t color, uint16_t *bg_color,
                                        uint16_t img_w, get_zhCN_dat get_font_data)
{
    uint8_t zhCN_dat[128]; //max 32x32 font

    memset(zhCN_dat, 0xff, sizeof(zhCN_dat));

    if (get_font_data)
    {
        get_font_data(zhCN_char, zhCN_dat, size);
    }

    image_rgb565_ram_draw_font_mat(ptr, 1, zhCN_dat, size, x, y, color, bg_color, img_w);

    return;
}

void image_rgb565_draw_zhCN_string(uint16_t *ptr, uint8_t *zhCN_string, uint8_t size,
                                   uint16_t x, uint16_t y,
                                   uint16_t color, uint16_t *bg_color,
                                   uint16_t img_w, uint16_t img_h,
                                   get_zhCN_dat get_font_data)
{
    uint8_t have_zhCN = 0;
    uint16_t ox = x;
    uint16_t oy = y;

    if ((size != 32 && size != 16) || (get_font_data == NULL))
    {
        return;
    }

    while (*zhCN_string != 0)
    {
        if (have_zhCN == 0)
        {
            if (*zhCN_string > 0x80)
            {
                have_zhCN = 1;
            }
            else
            {
                if (x > (ox + img_w - size / 2))
                {
                    y += size;
                    x = ox;
                }
                if (y > (oy + img_h - size))
                    break;

                if ((*zhCN_string == 0xd) ||
                    (*zhCN_string == 0xa))
                {
                    y += size;
                    x = ox;
                    zhCN_string++;
                }
                else
                {
                    image_rgb565_ram_draw_char(ptr, *zhCN_string, size, x, y, color, bg_color, img_w);
                }
                zhCN_string++;
                x += size / 2;
            }
        }
        else
        {
            have_zhCN = 0;
            if (x > (ox + img_w - size))
            {
                y += size;
                x = ox;
            }
            if (y > (oy + img_h - size))
                break;

            image_rgb565_draw_zhCN_char(ptr, zhCN_string, size, x, y, color, bg_color, img_w, get_font_data);
            zhCN_string += 2;
            x += size;
        }
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint16_t fast_blender_alpha(uint32_t x, uint32_t y, uint32_t alpha)
{
    x = (x | (x << 16)) & 0x7E0F81F;
    y = (y | (y << 16)) & 0x7E0F81F;
    uint32_t result = ((((x - y) * alpha) >> 5) + y) & 0x7E0F81F;
    return (uint16_t)((result & 0xFFFF) | (result >> 16));
}

void image_rgb565_mix_pic_with_alpha_lessmem(mix_image_t *img_src, mix_image_t *img_dst,
                                             uint32_t dst_addr, uint32_t alpha, uint8_t del_white,
                                             int (*flash_read_data)(uint32_t addr, uint8_t *data_buf, uint32_t length))
{
    uint16_t sx, sy, sw, sh;
    uint16_t dx, dy, dw, dh;

    sx = img_src->x;
    sy = img_src->y;
    sw = img_src->w;
    sh = img_src->h;

    dx = img_dst->x;
    dy = img_dst->y;
    dw = img_dst->w;
    dh = img_dst->h;

    if ((dx + dw) > (sw + sx) || (dh + dy) > (sh + sy))
    {
        // printk("[image_rgb565_mix_pic_with_alpha]:image invaild\r\n");
        return;
    }

    //申请一行的大小
    uint16_t *dst_buf = malloc(dw * 2);

    for (uint16_t i = dy; i < (dy + dh); i++)
    {
        //读取一行
        flash_read_data((uint32_t)(dst_addr + ((i - dy) * dw * 2)), (uint8_t *)dst_buf, dw * 2);

        for (uint16_t j = dx; j < (dx + dw); j++)
        {
            *(img_src->img_addr + i * sw + j) = fast_blender_alpha((uint32_t) * (img_src->img_addr + i * sw + j),
                                                                   (uint32_t) * ((uint16_t *)dst_buf + (j - dx)),
                                                                   (uint32_t)alpha / 8);
        }
    }

    if (dst_buf)
    {
        free(dst_buf);
    }

    return;
}

void image_rgb565_mix_pic_with_alpha(mix_image_t *img_src, mix_image_t *img_dst, uint32_t alpha, uint8_t del_white)
{
    uint16_t sx, sy, sw, sh;
    uint16_t dx, dy, dw, dh;

    sx = img_src->x;
    sy = img_src->y;
    sw = img_src->w;
    sh = img_src->h;

    dx = img_dst->x;
    dy = img_dst->y;
    dw = img_dst->w;
    dh = img_dst->h;

    if ((dx + dw) > (sw + sx) || (dh + dy) > (sh + sy))
    {
        // printk("[image_rgb565_mix_pic_with_alpha]:image invaild\r\n");
        return;
    }

    for (uint16_t i = dy; i < (dy + dh); i++)
    {
        for (uint16_t j = dx; j < (dx + dw); j++)
        {
            if (del_white)
            {
                if ((uint32_t) * (img_dst->img_addr + (i - dy) * dw + (j - dx)) == 0xFFFF)
                {
                    continue;
                }
            }
            *(img_src->img_addr + i * sw + j) = fast_blender_alpha((uint32_t) * (img_src->img_addr + i * sw + j),
                                                                   (uint32_t) * (img_dst->img_addr + (i - dy) * dw + (j - dx)),
                                                                   (uint32_t)alpha / 8);
        }
    }
    return;
}