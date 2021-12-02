Utilities for controlling a Roland SoundCanvas SC88ST-Pro.

by @novelistparty

# Variation Navigator
(note: only builds on macOS, but should be doable on Linux or Windows via platform specific SDL flags)

Lists the 16 Parts and their current assigned instrument variation. 

Navigate to the variation selection screen with Z+Right.

Use Up and Down to navigate the categories and then press Right to enter the listing. Navigate the listing with Up and Down. If you have a SC88ST-Pro connected via a USB Serial dongle, the variation will change as you browse. Select a variation with A. Return to the Part screen with Z+Left.

## Use on other platforms

The following is the setup required to use the menu on any other platform. The
1 bit screen buffer can then be displayed on your device's screen.

```
#include "bitmap_text.h"
#include "canvas.h"
#include "sc88stpro_menu.h"

/* application setup */
struct canvas1bit canvas = {
    .pix_w = SCREEN_WIDTH,
    .pix_h = SCREEN_HEIGHT,
    .pitch = BUFFER_PITCH,
    .buf = (uint8_t *)screenbuffer,
    ._size = sizeof(screenbuffer),
    .rotated = (SCREEN_WIDTH < SCREEN_HEIGHT ? 1 : 0)};
canvas_clear(&canvas);
struct kbd_input kio = {0};

while (1) {
    /* get key input and store in `kio` */

    /* compute fps */

    /* update the menu */
    menu_main(&kio, &canvas, fps, buf_err_msg);
    
    /* display canvas on screen via canvas->buf */
}
```

The port to an STM32L010RB with a Sharp LCD is in-progress.
