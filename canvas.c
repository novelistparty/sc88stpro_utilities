#include "canvas.h"

#include <assert.h>
#include <math.h>


#define NBITS_PER_WORD (8)
#define BITMASK(b) (0x80 >> ((b) % NBITS_PER_WORD))  /* needs to flip on other system. OR the LSB MSB implementation on this machine is broken (likely) */
#define BITSLOT(b) ((b) / NBITS_PER_WORD)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + NBITS_PER_WORD - 1) / NBITS_PER_WORD)


void canvas_clear(struct canvas1bit * c) {
    if (!c->buf) { return; }
    for (size_t i = 0; i < c->_size; i++) {
        c->buf[i] = 0xFF;
    }
}

static void canvas_draw_pixel(const unsigned x, const unsigned y, const unsigned color, struct canvas1bit * const restrict canvas) {
    if (x < 0) return;
    if (y < 0) return;
    
    if (canvas->rotated) {
        if (x >= canvas->pix_w) return;
        if (y >= canvas->pix_h) return;
        
        if (color) BITCLEAR(canvas->buf + y * canvas->pitch, x);
        else BITSET(canvas->buf + y * canvas->pitch, x);
    } else {
        if (x >= canvas->pix_h) return;
        if (y >= canvas->pix_w) return;
        
        if (color) BITCLEAR(canvas->buf + (canvas->pix_h - x) * canvas->pitch, y);
        else BITSET(canvas->buf + (canvas->pix_h - x) * canvas->pitch, y);
    }
    
    //    if (color) BITSET(canvas->buf, y + canvas->pix_h * x);
    //    else BITCLEAR(canvas->buf, y + canvas->pix_h * x);
}

void canvas_draw_rect(unsigned x1, unsigned x2, unsigned y1, unsigned y2, const unsigned color, struct canvas1bit * const restrict canvas) {
    /* loop over pixels */
    if (x1 > x2) {
        int tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    
    if (y1 > y2) {
        int tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    
    for (unsigned iy = y1; iy < y2; iy++) {
        for (unsigned ix = x1; ix < x2; ix++) {
            canvas_draw_pixel(ix, iy, color, canvas);
        }
    }
}

/* draw at the curent position. You must move the "cursor" */
void _canvas_draw_char(const char c, bmtx_context_t * ctx, struct canvas1bit * canvas) {
    const unsigned font_w = ctx->font_desc.width;
    const unsigned font_h = ctx->font_desc.height;
    const unsigned w_cell = font_w * ctx->hscale;
    const unsigned h_cell = font_h * ctx->vscale;
    
    /* canvas pixel coordinates, `pos` is in units of character cell size (8) */
    const int x1 = (ctx->origin.x + ctx->pos.x) * w_cell;
    const int y1 = (ctx->origin.y + ctx->pos.y) * h_cell;
    const int x2 = x1 + w_cell;
    const int y2 = y1 + h_cell;
    
    /* convert the bitmap font to pixels in a buffer */
    int offset = 8 * (c - ctx->font_desc.first_char);
    if (offset < 0) offset = 0;  // todo handle this somehwere else, upper bound too
    
    /* space */
    if (32 == c) {
        canvas_draw_rect(x1, x2, y1, y2, ctx->back_color, canvas);
        return;
    }
    
    /* the other characters */
    const uint8_t * const restrict char_data = ctx->font_desc.ptr + offset;
    int y = y1;
    for (size_t iy = 0; iy < font_h; iy++) {
        for (size_t iv = 0; iv < ctx->vscale; iv++) {
            if (iv > 0) { /* copy the last line */ }
            uint8_t this_byte = char_data[iy];
            size_t src_bit = 8;
            int x = x1;
            while (src_bit-- > (8 - font_w)) {
                const uint8_t bit = (this_byte >> src_bit) & 1;
                
                for (size_t ih = 0; ih < ctx->hscale; ih++) {
                    if (bit) { canvas_draw_pixel(x, y, ctx->txt_color, canvas); }
                    else     { canvas_draw_pixel(x, y, ctx->back_color, canvas); }
                    x++;
                }
            }
            y++;
        }
    }
}

void _bmtx_ctrl_char(uint8_t c, bmtx_context_t * ctx) {
    switch (c) {
        case '\r':
            ctx->pos.x = 0.0f;
            break;
        case '\n':
            ctx->pos.x = 0.0f;
            ctx->pos.y += 1.0f;
            break;
        case '\t':
            ctx->pos.x = (ctx->pos.x - fmodf(ctx->pos.x, ctx->tab_width)) + ctx->tab_width;
            break;
        case ' ':
            ctx->pos.x += 1.0f;
            break;
    }
}

void bmtx_putc(const char c, bmtx_context_t * ctx, struct canvas1bit * canvas) {
    assert(ctx->font_desc.width <= 8);
    uint8_t c_u8 = c;
    /* print a space for non included characters */
    if (32 < ctx->font_desc.first_char && 32 < c_u8 && c_u8 < ctx->font_desc.first_char) { c_u8 = ' '; }
    else if (c_u8 > ctx->font_desc.last_char) { c_u8 = ' '; }
    
    if (c_u8 < 32) {
        _bmtx_ctrl_char(c_u8, ctx);
    } else {
        _canvas_draw_char(c_u8, ctx, canvas);
        ctx->pos.x += 1;
    }
}

void bmtx_puts(const char* str, bmtx_context_t * ctx, struct canvas1bit * canvas) {
    char chr;
    while (0 != (chr = *str++)) {
        bmtx_putc(chr, ctx, canvas);
    }
}

void bmtx_puts_at_xy(const char * str, const float x, const float y, bmtx_context_t * ctx, struct canvas1bit * c) {
    ctx->pos.x = x;
    ctx->pos.y = y;
    bmtx_puts(str, ctx, c);
}
