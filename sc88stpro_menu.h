#include "canvas.h"

enum key_state { KEY_NOT_PRESSED = 0, KEY_PRESSED = 1, KEY_HELD = 5};

struct kbd_input {
    enum key_state up;
    enum key_state down;
    enum key_state left;
    enum key_state right;
    enum key_state select;
    enum key_state start;
    enum key_state A;
    enum key_state B;
};

void menu_main(struct kbd_input * kio, struct canvas1bit * canvas, unsigned fps, char buf_err_msg[]);
