#include <stdint.h>

void sysex_change_part_sound(uint8_t part, unsigned PN, unsigned n_vari);

void sysex_part_PAf_PITCH_CONTROL(uint8_t part, uint8_t val);
void sysex_part_CAf_PITCH_CONTROL(uint8_t part, uint8_t val);
