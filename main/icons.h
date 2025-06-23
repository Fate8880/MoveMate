#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>
#include "display.h"

// Icon bitmaps (1bpp, width=ICON_W, height=ICON_H)
extern const uint8_t foot_icon_bits[ICON_H * ((ICON_W + 7) / 8)];
extern const uint8_t x_icon_bits   [ICON_H * ((ICON_W + 7) / 8)];
extern const uint8_t fire_icon_bits[ICON_H * ((ICON_W + 7) / 8)];

// Dog sprites per state (1bpp, width=SPRITE_W, height=SPRITE_H)
extern const uint8_t *dog_sprite_bits[5];

#endif // ICONS_H
