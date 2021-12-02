#ifndef BITMAP_TEXT_H
#define BITMAP_TEXT_H

/*
 bitmap_text,
 fonts from sokol_debugtext.h, a bit of interface also

 other resources:
    SDL2 https://github.com/spurious/SDL-mirror/blob/e17aacbd09e65a4fd1e166621e011e581fb017a8/src/video/SDL_blit_copy.c#L91
    pitch: length of a line in bytes
    skip: usually set to pitch so you can memcpy on every new line
*/

#include "fonts.h"

#include <stddef.h>
#include <stdint.h>

/* each character must fit within an 8x8bit cell. It is 8 bytes in memory.
   smaller fonts (such as 5x5) are expected to be placed in the upper left of their cell (or wherever depending on their byte ordering
   at the moment, the fonts from sokol_debugtext are big-endian, this may change */
typedef struct {
    const uint8_t* ptr;
    size_t size; // byte size of font pixel data
    unsigned int width; // bit width of character within
    unsigned int height; // bit height of character
    unsigned int first_char; // decimal ascii number
    unsigned int last_char;  // decimal ascii number
} _bmtx_font_desc_t;

typedef struct {
    float x, y;
} _bmtx_vec2_t;

typedef struct {
    _bmtx_vec2_t pos;    /* units of character widths */
    _bmtx_vec2_t origin; /* units of character widths */
    unsigned int hscale;        /* integer scaling */
    unsigned int vscale;        /* integer scaling */
    unsigned int tab_width;
    unsigned int txt_color;
    unsigned int back_color;
    _bmtx_font_desc_t font_desc;
} bmtx_context_t;

void bmtx_set_colors(bmtx_context_t * b, unsigned txt_color, unsigned back_color);
/*
 sdtx_origin(x, y);
 sdtx_pos(x, y)      - sets absolute cursor position
 sdtx_pos_x(x)       - only set absolute x cursor position
 sdtx_pos_y(y)       - only set absolute y cursor position
 sdtx_move(x, y)     - move cursor relative in x and y direction
 sdtx_move_x(x)      - move cursor relative only in x direction
 sdtx_move_y(y)      - move cursor relative only in y direction
 sdtx_crlf()         - set cursor to beginning of next line
                       (same as sdtx_pos_x(0) + sdtx_move_y(1))
 sdtx_home()         - resets the cursor to the origin
                       (same as sdtx_pos(0, 0))
 sdtx_color3b(r, g, b)       - RGB 0..255, A=255
 sdtx_putc(c)             - output a single character
 sdtx_puts(str)           - output a null-terminated C string
 sdtx_putr(str, len)     - 'put range' output the first 'len' characters of
                            a C string or until the zero character is encountered
 */
/* hey john, don't make this general for software and hardware yet
 get it working properly and then do the port */

/* hey john */

#endif /* BITMAP_TEXT_H */
