#ifndef CANVAS_H
#define CANVAS_H

#include "bitmap_text.h"

#include <stddef.h>
#include <stdint.h>

struct canvas1bit {
    unsigned pix_w;
    unsigned pix_h;
    unsigned pitch;
    unsigned rotated;
    size_t _size; /* in bytes */
    uint8_t * buf;
};

void canvas_clear(struct canvas1bit * c);

void canvas_draw_rect(unsigned x1, unsigned x2, unsigned y1, unsigned y2, const unsigned color, struct canvas1bit * const restrict canvas);

void _canvas_draw_char(const char c, bmtx_context_t * ctx, struct canvas1bit * canvas);

void bmtx_puts(const char* str, bmtx_context_t * ctx, struct canvas1bit * canvas);

void bmtx_puts_at_xy(const char * str, const float x, const float y, bmtx_context_t * ctx, struct canvas1bit * c);

#endif /* CANVAS_H */
