#include "icons.h"

// Calculate raw byte sizes
#define ICON_BYTES   (ICON_H      * ((ICON_W    + 7) / 8))
#define SPRITE_BYTES (SPRITE_H    * ((SPRITE_W  + 7) / 8))

// Fill each byte with 0xFF so every bit is 'on' → solid block in GREEN
const uint8_t foot_icon_bits[ICON_BYTES]    = { [0 ... ICON_BYTES-1] = 0xFF };
const uint8_t x_icon_bits   [ICON_BYTES]    = { [0 ... ICON_BYTES-1] = 0xFF };
const uint8_t fire_icon_bits[ICON_BYTES]    = { [0 ... ICON_BYTES-1] = 0xFF };

// One dummy sprite, reused for all 5 states
static const uint8_t dog_dummy[SPRITE_BYTES] = { [0 ... SPRITE_BYTES-1] = 0xFF };
const uint8_t *dog_sprite_bits[5] = {
    dog_dummy, dog_dummy, dog_dummy, dog_dummy, dog_dummy
};
