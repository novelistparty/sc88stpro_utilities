#include "sc88stpro_menu.h"

#include "sc88stpro_map.h"
#include "sc88stpro_midi_sysex.h"

#include <stdint.h>
#include <stdio.h>

bmtx_context_t bmtx1 = {
    .hscale = 1,
    .vscale = 1,
    .font_desc = {
        _sdtx_font_oric,
        sizeof(_sdtx_font_oric),
        7, 8, 33, 126},
    .tab_width = 2
};

bmtx_context_t bmtx2 = {
    .hscale = 1,
    .vscale = 1,
    .font_desc = {
        _bmtx_font_cgpixel5x5,
        sizeof(_bmtx_font_cgpixel5x5),
        5, 6, 33, 126},
    .tab_width = 2
};

enum menu_mvmt { MENU_MV_NONE = 0, MENU_MV_UP, MENU_MV_DOWN, MENU_MV_LEFT, MENU_MV_RIGHT };
static enum menu_mvmt menu_mvmt = MENU_MV_NONE;
enum menu_mods { MOD_NONE = 0, MOD_SELECT, MOD_START, MOD_A, MOD_B};
//static enum menu_mods menu_mod = MOD_NONE;

/* part list menu vars */
uint8_t current_part_list_part_idx = 0;

/* instrument list menu vars */
uint8_t current_instr_group_idx = 0;
uint8_t current_vari_idx = 0;
uint8_t current_instr_menu_col = 0;

enum display {SHOW_PART_LIST, SHOW_INSTRUMENT_LIST} current_menu = SHOW_PART_LIST, last_menu = SHOW_PART_LIST;

/* defaults, not meant to be set by hand here */
struct current_part {
    unsigned instrument_group;
    unsigned PN;
    unsigned var_n;
} parts_list_B[16] = {
    { 0,   1, 16},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1},
    {14, 123, 1}
};

void sc88_get_variation_name(char buf[], size_t N, const size_t ig, const size_t PN, const size_t varN) {
    if (ig >= sc88map.N) {
        snprintf(buf, N, "This index shall not pass");
    }
    struct _sc88_instrument this_instr = sc88map.instr[ig];
    for (size_t i = 0; i < this_instr.Nvars; i++) {
        struct _sc88_variation this_vari = this_instr.variations[i];
        if ((PN == this_vari.PN) && (varN == this_vari.n_var)) {
            snprintf(buf, N, "%s", this_vari.name);
        }
    }
}

char buf[64];

void menu_main(struct kbd_input * kio, struct canvas1bit * canvas, unsigned fps, char buf_err_msg[]) {
    /* resolve the active input to the menu input */
    {
        menu_mvmt = MENU_MV_NONE; /* reset */
        const uint8_t ksum = kio->up + kio->down + kio->left + kio->right;
        if (ksum > 1) { /* too many new key downs at once, bail */
            menu_mvmt = MENU_MV_NONE;
        } else if (1 == ksum) {
            if      (kio->up)                   { menu_mvmt = MENU_MV_UP;    }
            else if (kio->down)                 { menu_mvmt = MENU_MV_DOWN;  }
            else if (KEY_PRESSED == kio->left)  { menu_mvmt = MENU_MV_LEFT;  }
            else if (KEY_PRESSED == kio->right) { menu_mvmt = MENU_MV_RIGHT; }
        }
    }
    
    canvas_clear(canvas);
    
    /* similar structure to LSDJ. Mainline Left-Right with option screens for each above and below */
    
    /* Main screen -> Parts List. Above: Global FX*/
    
    /* "Select + Right" - Moving from Part List to Variation list */
    if ((SHOW_PART_LIST == current_menu) && (kio->select) && (MENU_MV_RIGHT == menu_mvmt)) {
        current_menu = SHOW_INSTRUMENT_LIST;
        current_instr_group_idx = parts_list_B[current_part_list_part_idx].instrument_group;
    }
    /* "Select + Up" - Moving from Part List to Variation list */
    else if ((SHOW_PART_LIST == current_menu) && (kio->select) && (MENU_MV_RIGHT == menu_mvmt)) {
        current_menu = SHOW_INSTRUMENT_LIST;
        current_instr_group_idx = parts_list_B[current_part_list_part_idx].instrument_group;
    }
    /* "Select + Left" - Moving from Variation List to Part List */
    else if ((SHOW_INSTRUMENT_LIST == current_menu) && kio->select && (MENU_MV_LEFT == menu_mvmt)) {
        current_menu = SHOW_PART_LIST;
        
        const struct _sc88_instrument this_instr = sc88map.instr[current_instr_group_idx];
        const size_t Nvars = this_instr.Nvars;
        const size_t n_var = parts_list_B[current_part_list_part_idx].var_n;
        const size_t PN = parts_list_B[current_part_list_part_idx].PN;
        for (size_t i = 0; i < Nvars; i++) {
            if ((n_var == this_instr.variations[i].n_var)
                && (PN == this_instr.variations[i].PN)) {
                current_vari_idx = i;
                break;
            }
        }
    }
    
    /* Part Listing */
    if (SHOW_PART_LIST == current_menu) {
        /* cycle through the part listing */
        if (MENU_MV_UP   == menu_mvmt) { current_part_list_part_idx = (current_part_list_part_idx - 1) & 15; }
        if (MENU_MV_DOWN == menu_mvmt) { current_part_list_part_idx = (current_part_list_part_idx + 1) & 15; }
        
        /* Display the Part Labels, A01 - A16 */
        bmtx1.vscale = 1; bmtx1.hscale = 1;
        bmtx1.origin.x = 0.2f; bmtx1.origin.y = 1.0f;
        bmtx1.pos.x = 0.0;     bmtx1.pos.y = 0.0;
        for (size_t i = 0; i < 16; i++) {
            snprintf(buf, 64, "A%02zu", i + 1);
            bmtx_puts(buf, &bmtx1, canvas);
            bmtx1.pos.x = 0.0;
            bmtx1.pos.y += 3.8;
            
            if (7 == i) {
                bmtx1.origin.x += 18;
                bmtx1.pos.y = 0.0f;
            }
        }
        
        /* Display the Current Part Instrument?/Variation? */
        bmtx1.vscale = 2; bmtx1.hscale = 1;
        bmtx1.origin.x = 4.0f; bmtx1.origin.y = 0.4f;
        bmtx1.pos.x = 0.0f;    bmtx1.pos.y = 0.0f;
        for (size_t i = 0; i < 16; i++) {
            /* highlight the current one */
            if (i == current_part_list_part_idx) { bmtx_set_colors(&bmtx1, 0, 1); }
            
            char this_name[32];
            const struct current_part instr = parts_list_B[i];
            sc88_get_variation_name(this_name, 32, instr.instrument_group, instr.PN, instr.var_n);
            bmtx_puts(this_name, &bmtx1, canvas);
            bmtx1.pos.x = 0.0f;
            bmtx1.pos.y += 1.9;
            bmtx_set_colors(&bmtx1, 1, 0);
            
            if (7 == i) {
                bmtx1.origin.x += 18.0f;
                bmtx1.pos.y = 0.0f;
            }
        }
    }
    
    /* Instrument?/Variation? List */
    if (SHOW_INSTRUMENT_LIST == current_menu) {
        /* starting on left column */
        if (0 == current_instr_menu_col) {
            /* moving to right column */
            if (MENU_MV_RIGHT == menu_mvmt && (last_menu == current_menu)) {
                current_instr_menu_col = 1;
            }
            /* moving up through the column */
            else if (MENU_MV_UP == menu_mvmt) {
                if (0 != current_instr_group_idx) {
                    current_instr_group_idx -= 1;
                    current_vari_idx = 0;
                }
            }
            /* moving down through the column */
            else if (MENU_MV_DOWN == menu_mvmt) {
                if (15 != current_instr_group_idx) {
                    current_instr_group_idx += 1;
                    current_vari_idx = 0;
                }
            }
        }
        /* starting on the right column */
        else if (1 == current_instr_menu_col) {
            const size_t Nvari = sc88map.instr[current_instr_group_idx].Nvars;
            /* moving to the left column */
            if (MENU_MV_LEFT == menu_mvmt) {
                current_instr_menu_col = 0;
            }
            /* moving up through the left column */
            else {
                int changed = 0;
                if (MENU_MV_UP == menu_mvmt) {
                current_vari_idx = (0 == current_vari_idx) ? 0 : (current_vari_idx - 1);
                    changed = 1;
                }
                else if (MENU_MV_DOWN == menu_mvmt) {
                    current_vari_idx = (current_vari_idx == (Nvari - 1)) ? current_vari_idx : (current_vari_idx + 1);
                    changed = 1;
                }
                if (changed) {
                parts_list_B[current_part_list_part_idx].var_n = sc88map.instr[current_instr_group_idx].variations[current_vari_idx].n_var;
                parts_list_B[current_part_list_part_idx].PN = sc88map.instr[current_instr_group_idx].variations[current_vari_idx].PN;
                parts_list_B[current_part_list_part_idx].instrument_group = current_instr_group_idx;
                sysex_change_part_sound(current_part_list_part_idx + 1, parts_list_B[current_part_list_part_idx].PN, parts_list_B[current_part_list_part_idx].var_n);
                }
            }

            /* set this for this Part */
            if ((KEY_PRESSED == kio->A)
                && (current_vari_idx != parts_list_B[current_part_list_part_idx].var_n)) {
                parts_list_B[current_part_list_part_idx].var_n = sc88map.instr[current_instr_group_idx].variations[current_vari_idx].n_var;
                parts_list_B[current_part_list_part_idx].PN = sc88map.instr[current_instr_group_idx].variations[current_vari_idx].PN;
                parts_list_B[current_part_list_part_idx].instrument_group = current_instr_group_idx;
                sysex_change_part_sound(current_part_list_part_idx + 1, parts_list_B[current_part_list_part_idx].PN, parts_list_B[current_part_list_part_idx].var_n);
            }
        }
        /* track the column value, 0 or 1, for highlighting
         - show full list of groups, icon showing current one when not highlighted
         - have selection Icon maybe > Synth1 < in the list
         - scrolling list, index # small in front of each
         
         Current: 123, 001
         SynthFX,  Rain
         
         To audition, move around
         To set your choice, Press A
         ---------------------------
         Piano         Acoustic Bs.
         Keys          Rockabilly
         Brass         Wild A.Bass
         >Bass        >Bass + OHH
         Synth         Picked Bass
         SFX           Picked Bass 2
         
         teeny text below showing the PN, CC#0
         the MIDI message
         
         */
        
        /* group list - highlight the current */
        /* set a local tracking variable for group */
        const struct current_part part = parts_list_B[current_part_list_part_idx];
        const size_t instr_idx = part.instrument_group;
        
        /* instr group list */
        bmtx1.vscale = 2; bmtx1.hscale = 1;
        bmtx1.origin.x = 1.25f; bmtx1.origin.y = 0.5f;
        bmtx1.pos.x = 0.0f; bmtx1.pos.y = 0.0f;
        for (size_t i = 0; i < sc88map.N; i++) {
            // mark current with >
            if (i == instr_idx) {
                bmtx_puts_at_xy(">", -1.0f, bmtx1.pos.y, &bmtx1, canvas);
            }
            // reverse color of current
            if (i == current_instr_group_idx) {
                bmtx_set_colors(&bmtx1, 0, 1);
            }
            
            bmtx_puts(sc88map.instr[i].name, &bmtx1, canvas);
            bmtx1.pos.x = 0.0f;
            bmtx1.pos.y++;
            bmtx_set_colors(&bmtx1, 1, 0);
        }
        
        /* instrument group variation list */
        bmtx1.vscale = 2;
        bmtx1.origin.x = 14.25f; bmtx1.origin.y = 0.5f;
        bmtx1.pos.x = 0.0f; bmtx1.pos.y = 0.0f;
        const struct _sc88_instrument this_instr = sc88map.instr[current_instr_group_idx];
        const size_t var_n = part.var_n;
        const size_t PN = part.PN;
        
        /* display a portion of the variation listing */
        size_t istart = 0;
        const size_t Ndisp = 16;
        if (current_vari_idx < 15) {
            istart = 0;
        } else {
            istart = (current_vari_idx - 15);
            /* bounds check for istart+Ndisp handled in update of current_vari_idx */
        }
        
        
        for (size_t i = istart; i < (istart + Ndisp); i++) {
            const struct _sc88_variation this_vari = this_instr.variations[i];
            if ((var_n == this_vari.n_var)
                && (PN == this_vari.PN)) {
                bmtx_puts_at_xy(">", -1.0f, bmtx1.pos.y, &bmtx1, canvas);
            }
            
            if (i == current_vari_idx) {
                bmtx_set_colors(&bmtx1, 0, 1);
            }
            
            bmtx_puts(sc88map.instr[current_instr_group_idx].variations[i].name, &bmtx1, canvas);
            bmtx1.pos.x = 0.0f;
            bmtx1.pos.y++;
            bmtx_set_colors(&bmtx1, 1, 0);
        }
    }
    
    last_menu = current_menu;
    
    /* FPS */
    snprintf(buf, 64, "%d FPS", fps);
    bmtx1.vscale = 2;
    bmtx1.origin.x = 0.0f; bmtx1.origin.y = 19.0f;
    bmtx1.pos.x = 0.0f; bmtx1.pos.y = 0.0f;
    bmtx_puts_at_xy(buf, 0.0f, 0.0f, &bmtx1, canvas);

    /* Error Messages */
    bmtx1.vscale = 1;
    bmtx1.origin.x = 0.0f; bmtx1.origin.y = 0.0f;
    bmtx_puts_at_xy(buf_err_msg, 0.0f, 45.f, &bmtx1, canvas);
    
    snprintf(buf, 64, "MVMT KEY: %s %s %s %s",
             (kio->up ? "UP" : "  "),
             (kio->down ? "DOWN" : "    "),
             (kio->left ? "LEFT" : "    "),
             (kio->right ? "RIGHT" : "    "));
    bmtx1.vscale = 2;
    bmtx1.origin.x = 0.0f; bmtx1.origin.y = 0.0f;
    bmtx_puts_at_xy(buf, 0.0f, 20.0f, &bmtx1, canvas);
    
    snprintf(buf, 64, "MOD KEY: %s %s %s %s",
             (kio->A ? "A" : " "),
             (kio->B ? "B" : " "),
             (kio->select ? "SELECT" : "      "),
             (kio->start ? "START" : "     "));
    bmtx1.vscale = 2;
    bmtx1.origin.x = 0.0f; bmtx1.origin.y = 0.0f;
    bmtx_puts_at_xy(buf, 0.0f, 21.0, &bmtx1, canvas);
}

/* The main section of the manual is written around concepts of Part and Variation. The MIDI Implementation in the Appendix refers to the unique sound that is given by an Instrument number and a Variation number as a "Tone." */

/* Patch Part Parameters (from Appendix of SC88ST-Pro Manual)
" Parameters that can be set individually for each Part are called Patch Part Parameters
 */

/*
 //    unsigned going_up = 0;
 //    int count = 0;
 while
 
 #if 0
        if (going_up) {
            if (count > 100) going_up = 0;
            count++;
        }
        if (!going_up) {
            count--;
            if (count < 1) going_up = 1;
        }
        canvas_clear(canvas);
        canvas_draw_rect(count, 102, count, 102, 1, canvas);
#else
*/
