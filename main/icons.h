#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>
#include "display.h"

/* OLD PLACEHOLDER
// Icon bitmaps (1bpp, width=ICON_W, height=ICON_H)
extern const uint8_t foot_icon_bits[ICON_H * ((ICON_W + 7) / 8)];
extern const uint8_t x_icon_bits   [ICON_H * ((ICON_W + 7) / 8)];
extern const uint8_t fire_icon_bits[ICON_H * ((ICON_W + 7) / 8)];

// Dog sprites per state (1bpp, width=SPRITE_W, height=SPRITE_H)
extern const uint8_t *dog_sprite_bits[6];
*/

extern const uint16_t foot_icon_pixels[];
#define FOOT_ICON_W 32
#define FOOT_ICON_H 32
extern const uint16_t x_icon_pixels[];
#define X_ICON_W 32
#define X_ICON_H 32
extern const uint16_t fire_icon_pixels[];
#define FIRE_ICON_W 32
#define FIRE_ICON_H 32
extern const uint16_t dead_icon_pixels[];
#define DEAD_ICON_W 64
#define DEAD_ICON_H 64
extern const uint16_t excited_icon_pixels[];
#define EXCITED_ICON_W 64
#define EXCITED_ICON_H 64
extern const uint16_t happy_icon_pixels[];
#define HAPPY_ICON_W 64
#define HAPPY_ICON_H 64
extern const uint16_t neutral_icon_pixels[];
#define NEUTRAL_ICON_W 64
#define NEUTRAL_ICON_H 64
extern const uint16_t running_icon_pixels[];
#define RUNNING_ICON_W 64
#define RUNNING_ICON_H 64
extern const uint16_t sad_icon_pixels[];
#define SAD_ICON_W 64
#define SAD_ICON_H 64
extern const uint16_t strong_icon_pixels[];
#define STRONG_ICON_W 64
#define STRONG_ICON_H 64
extern const uint16_t walking_icon_pixels[];
#define WALKING_ICON_W 64
#define WALKING_ICON_H 64
extern const uint16_t weak_icon_pixels[];
#define WEAK_ICON_W 64
#define WEAK_ICON_H 64


#endif // ICONS_H
