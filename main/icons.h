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

extern const uint16_t foot_icon_pixels[];
#define FOOT_ICON_W 32
#define FOOT_ICON_H 32
extern const uint16_t x_icon_pixels[];
#define X_ICON_W 32
#define X_ICON_H 32
extern const uint16_t fire_icon_pixels[];
#define FIRE_ICON_W 32
#define FIRE_ICON_H 32
/*extern const uint16_t dog1_icon_pixels[];
#define DOG1_ICON_W 64
#define DOG1_ICON_H 64*/


#endif // ICONS_H
