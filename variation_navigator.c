#include "macros.h"
#include "bitmap_text.h"
#include "canvas.h"
#include "sc88stpro_menu.h"
#include "sc88stpro_midi_sysex.h"

#include <SDL.h>

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <unistd.h>

#if 0
#define SCREENWIDTH 400
#define SCREENHEIGHT 240
#else
#define SCREENWIDTH 240
#define SCREENHEIGHT 400
#endif
#define MAXSCALE 3

int fd = -10;

char buf_err_msg[64] = {0};

void _midi_send(const void * __buf, size_t __nbytes) {
    if (fd < 0) { return; }
    
    unsigned char * buf = (unsigned char *)__buf;
    if (__nbytes > 10) {
    snprintf(buf_err_msg, 64, "%02hhx %02hhx %02hhx %02hhx %02hhx\n%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10]);
    }
    
    write(fd, __buf, __nbytes);
}

/* can be set to anything, but as long as vsync is enabled, can't go higher than vsync */
const int SCREEN_FPS = 20;
const uint32_t TICKS_PER_FRAME = 1e3 / SCREEN_FPS;

int main(int argc, char * argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    
    /* scale here to make the window larger than the surface itself. Integer scalings only as set by function below */
    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED, 2*SCREENWIDTH,2*SCREENHEIGHT, 0);
    
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    const int width = SCREENWIDTH; //
    const int height = SCREENHEIGHT;
    
    /* Since we are going to display a low resolution buffer, it is best to limit
     the window size so that it cannot be smaller than our internal buffer size. */
    SDL_SetWindowMinimumSize(window, width, height);
    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_RenderSetIntegerScale(renderer, 1);
    
    SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, width, height, 1, SDL_PIXELFORMAT_INDEX1MSB);
    
    SDL_Color colors[2] = {{0,0,0,255}, {255,255,255,255}};
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);
    
    struct canvas1bit canvas = {.pix_w = SCREENWIDTH, .pix_h = SCREENHEIGHT, .pitch = surface->pitch, ._size = surface->pitch * height, .buf = surface->pixels, .rotated = (SCREENWIDTH < SCREENHEIGHT ? 1 : 0)};
    canvas_clear(&canvas);
    fprintf(stderr, "Screen pitch in bytes: %d\n", surface->pitch);
    fprintf(stderr, "Expected pixw / 8: %d\n", width / 8);

    char * path_serial_device = "/dev/cu.usbserial-1130";
    int serial_device_status = access(path_serial_device, F_OK);
    int serial_device_found = (0 == serial_device_status) ? 1 : 0;
    if (!serial_device_found) {
        snprintf(buf_err_msg, 64, "WARNING serial device not found:\n%s", strerror(errno));
        fprintf(stderr, "%s\n", buf_err_msg);
    }
    
    struct kbd_input kio = {0};
    size_t quit = 0;
    Uint64 start = 0;
    while (!quit) {
        /* PLATFORM - frame start */
        const uint32_t frame_start = SDL_GetTicks();
        
        /* track buttons held since last frame */
        if (KEY_PRESSED == kio.up)         kio.up = KEY_HELD;
        if (KEY_PRESSED == kio.down)     kio.down = KEY_HELD;
        if (KEY_PRESSED == kio.left)     kio.left = KEY_HELD;
        if (KEY_PRESSED == kio.right)   kio.right = KEY_HELD;
        if (KEY_PRESSED == kio.A)           kio.A = KEY_HELD;
        if (KEY_PRESSED == kio.B)           kio.B = KEY_HELD;
        if (KEY_PRESSED == kio.select) kio.select = KEY_HELD;
        if (KEY_PRESSED == kio.start)   kio.start = KEY_HELD;

        /* PLATFORM - events */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (SDL_QUIT == event.type) quit = 1;
            if (SDL_KEYDOWN == event.type) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = 1; break;
                    case SDLK_UP:    if (KEY_NOT_PRESSED == kio.up)         kio.up = KEY_PRESSED; break;
                    case SDLK_DOWN:  if (KEY_NOT_PRESSED == kio.down)     kio.down = KEY_PRESSED; break;
                    case SDLK_LEFT:  if (KEY_NOT_PRESSED == kio.left)     kio.left = KEY_PRESSED; break;
                    case SDLK_RIGHT: if (KEY_NOT_PRESSED == kio.right)   kio.right = KEY_PRESSED; break;
                    case SDLK_a:     if (KEY_NOT_PRESSED == kio.A)           kio.A = KEY_PRESSED; break;
                    case SDLK_s:     if (KEY_NOT_PRESSED == kio.B)           kio.B = KEY_PRESSED; break;
                    case SDLK_z:     if (KEY_NOT_PRESSED == kio.select) kio.select = KEY_PRESSED; break;
                    case SDLK_x:     if (KEY_NOT_PRESSED == kio.start)   kio.start = KEY_PRESSED; break;
                }
            }
            if (SDL_KEYUP == event.type) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:       kio.up = KEY_NOT_PRESSED; break;
                    case SDLK_DOWN:   kio.down = KEY_NOT_PRESSED; break;
                    case SDLK_LEFT:   kio.left = KEY_NOT_PRESSED; break;
                    case SDLK_RIGHT: kio.right = KEY_NOT_PRESSED; break;
                    case SDLK_a:         kio.A = KEY_NOT_PRESSED; break;
                    case SDLK_s:         kio.B = KEY_NOT_PRESSED; break;
                    case SDLK_z:    kio.select = KEY_NOT_PRESSED; break;
                    case SDLK_x:     kio.start = KEY_NOT_PRESSED; break;
                }
            }
        }
        
        /* PLATFORM - Setup Serial Device */
        if ((KEY_PRESSED == kio.start) && (fd < 0) && serial_device_found) {
            
            fd = open(path_serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
            if (-1 == fd) {
                snprintf(buf_err_msg, 64, "ERROR opening serial device:\n%s", strerror(errno));
                goto serial_setup_end;
            }
            struct termios tty;
            if (0 != tcgetattr(fd, &tty)) {
                fprintf(stderr, "err %i from tcgetattr: %s\n", errno, strerror(errno));
                goto serial_setup_end;
            }
            
            tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
            tty.c_cflag |= CS8 | CLOCAL | CREAD;
            
            cfsetispeed(&tty, B38400);
            cfsetospeed(&tty, B38400);
            
            if (0 != tcsetattr(fd, TCSANOW, &tty)) {
                fprintf(stderr, "err %i from tcsetattr: %s\n", errno, strerror(errno));
                goto serial_setup_end;
            }
            
            serial_setup_end:;
        }
        
        /* PLATFORM - timing */
        Uint64 end = SDL_GetPerformanceCounter();
        const float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
        start = SDL_GetPerformanceCounter();
        unsigned fps = 1.0f / elapsed;

        /* Menu */
        menu_main(&kio, &canvas, fps, buf_err_msg);
       
        /* PLATFORM - press B to play a midi test (blocking) */
        if ((KEY_PRESSED == kio.B) && (fd > 0)) {
            sysex_part_PAf_PITCH_CONTROL(1, 0x40);
            sysex_part_CAf_PITCH_CONTROL(1, 0x00);
//            char msg_on[] = {0xB0, 0x5B, 0x00,
//                0xC0, 0x00,
//                0x90, 0x3C, 0x7F,
//                0x90, 0x3F, 0x7F};
//            char msg_on2[] = {0xC0, 0x32,
//                0x90, 0x30, 0x7F,
//                0x90, 0x34, 0x7F};
//            char msg_off[] = {0x90, 0x3C, 0x00,
//                0x90, 0x3F, 0x00,
//                0x90, 0x30, 0x00,
//                0x90, 0x34, 0x00
//            };
//
//            char all_off[] = {0xB0, 0x78, 0x00};
//
//            _midi_send(msg_on, sizeof(msg_on)/sizeof(char));
//            sleep(1);
//            _midi_send(msg_on2, sizeof(msg_on2)/sizeof(char));
//            sleep(1);
//            _midi_send(msg_off, sizeof(msg_off)/sizeof(char));
//            sleep(1);
//            _midi_send(all_off, sizeof(all_off)/sizeof(char));
        }
        
        /* PLATFORM - Render */
        SDL_RenderClear(renderer);
        SDL_Texture * screen_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(screen_texture);
        
        /* PLATFORM - FPS limit */
        const uint32_t frame_ticks = SDL_GetTicks() - frame_start;
        if (frame_ticks < TICKS_PER_FRAME) {
            SDL_Delay(TICKS_PER_FRAME - frame_ticks);
        }
    }
}

/*
 Platform Specific Implementations
 * _midi_send()
 *
 */
