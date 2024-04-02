#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fmath.h"
#include "imdefs.h"
// #include "imlib.h"


enum FontIndex {
    ASCII,
    Unicode,
    UTF8,
    GBK,
    GB2312,
};

enum FontSource {
    BuildIn,
    FileIn,
    StringIO,
    ArrayIn,
};

//source_type default ArrayIn
void font_load(uint8_t index, uint8_t width, uint8_t high, uint8_t source_type, void *src_addr);
void font_free();


#endif // !_FONT_H