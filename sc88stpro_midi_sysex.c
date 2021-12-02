#include "sc88stpro_midi_sysex.h"

#include <stdio.h>
#include <stdlib.h>

/* define midi_send elsewhere */
extern void _midi_send(const void * __buf, size_t __nbytes);

static uint8_t sc88stpro_part_to_block_number(const uint8_t n_part) {
    uint8_t val = 0;
    if ((0 == n_part) || (n_part > 16)) {
        fprintf(stderr, "%s ERROR: sc88 parts are 1-16 inclusive, you gave %hu\n", __func__, n_part);
    }
    
    if (10 == n_part) val = 0;
    else val = n_part;
    return val;
}

void sysex_change_part_sound(uint8_t part, unsigned PN, unsigned n_vari) {
    /* F0 41 10 42 12 address data sum F7
     exclusive status, ID number (roland), Device ID, Model ID (GS), Command ID (DT1), address, data, checksum, EOX End of Exclusive
     address is 40 xx xx for same bank messages, 50 xx xx to the other
     data is */
    uint8_t msg[12] = {0xF0, 0x41, 0x10, 0x42, 0x12,
        0x50, 0x10 + sc88stpro_part_to_block_number(part), 0x00,
        n_vari, PN - 1, 0x00, 0xF7};
    /* Roland SysEx checksum is the two's complement of the sum of address and size bytes */
    uint8_t sum = msg[5] + msg[6] + msg[7] + msg[8] + msg[9];
    sum %= 128;
    msg[10] = 128 - sum;
    
    _midi_send(msg, sizeof(msg)/sizeof(uint8_t));
}

/* The Patch Part Parameters BEND, MOD, CAf, PAf, CC1, CC2 each have several CONTROL values for parameters such as PITCH, TVF CUTOFF, AMPLITUDE, LFO RATE. A CONTROL parameter gives the range that parameter can change.

As an example, the Pitch Bend is enabled by default for a range of two semitones. Messages from the Pitch Bend wheel can use that range. */
/*
 SysEx Data Set 1 - DT1(12h)
 
 F0h Exclusive Status
 41h ID Number (Roland)
 10h Device ID
 42h Model ID (GS) - also Device Family Code
 12h Command ID (DT1)
 aah Address MSB
 bbh Address
 cch Address LSB
 ddh Data
 .
 .
 eeh Data
 sum Checksum
 F7h EOX (End Of Exclusive)
 */

/* big ol' table of the midi implementation
 Name, Display range [min,max], value range, units (descriptive), default, address, Size, Description
 */

uint8_t sysex_data_set1_preamble[5] = {0xF0, 0x41, 0x10, 0x42, 0x12};

/* sizes:
 5 preamble
 3 address
 1-4 data (see note below)
 1 checksum
 1 EOX End of Exclusive
 
 Note: there are two 12 byte data items: "drum map name" and "scale tuning." The first is read-only (jkb: maybe?))*/
uint8_t _sysex_dt1_buf[5+3+4+1+1];

uint8_t sysex_checksum(uint8_t address[3], uint8_t val[], uint8_t N) {
    uint8_t sum = address[0] + address[1] + address[2];
    for (size_t i = 0; i < N; i++) {
        sum += val[i];
    }
    sum %= 128;
    return 128 - sum;
}

static uint8_t clamp(uint8_t minmax[2], uint8_t val) {
    if (val < minmax[0]) val = minmax[0];
    if (val > minmax[1]) val = minmax[1];
    return val;
}

void sysex_part_PAf_PITCH_CONTROL(uint8_t part, uint8_t val) {
    uint8_t address[3] = {0x50, 0x20, 0x30};
    address[1] += sc88stpro_part_to_block_number(part);
    uint8_t minmax[2] = {0x28, 0x58};
    val = clamp(minmax, val);
    
    unsigned int idx = 0;
    for (size_t i = 0; i < 5; i++) {
        _sysex_dt1_buf[idx++] = sysex_data_set1_preamble[i];
    }
    _sysex_dt1_buf[idx++] = address[0];
    _sysex_dt1_buf[idx++] = address[1];
    _sysex_dt1_buf[idx++] = address[2];

    _sysex_dt1_buf[idx++] = val; // data
    _sysex_dt1_buf[idx++] = sysex_checksum(address, &val, 1);
    _sysex_dt1_buf[idx++] = 0xF7;

    _midi_send(_sysex_dt1_buf, idx);
}

void sysex_part_CAf_PITCH_CONTROL(uint8_t part, uint8_t val) {
    uint8_t address[3] = {0x50, 0x20, 0x20};
    address[1] += sc88stpro_part_to_block_number(part);
    uint8_t minmax[2] = {0x28, 0x58};
    val = clamp(minmax, val);
    
    unsigned int idx = 0;
    for (size_t i = 0; i < 5; i++) {
        _sysex_dt1_buf[idx++] = sysex_data_set1_preamble[i];
    }
    _sysex_dt1_buf[idx++] = address[0];
    _sysex_dt1_buf[idx++] = address[1];
    _sysex_dt1_buf[idx++] = address[2];

    _sysex_dt1_buf[idx++] = val; // data
    _sysex_dt1_buf[idx++] = sysex_checksum(address, &val, 1);
    _sysex_dt1_buf[idx++] = 0xF7;

    _midi_send(_sysex_dt1_buf, idx);
}
