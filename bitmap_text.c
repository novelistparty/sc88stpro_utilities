#include "bitmap_text.h"

void bmtx_set_colors(bmtx_context_t * b, unsigned txt_color, unsigned back_color) {
    b->txt_color = txt_color;
    b->back_color = back_color;
}
