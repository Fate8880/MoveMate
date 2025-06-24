// display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "st7789.h"
#include "fontx.h"
#include "stepcounter.h"   // for movement_state_t

// Dimensions (adjust to your actual bitmaps)
#define ICON_W     32
#define ICON_H     32
#define SPRITE_W   64
#define SPRITE_H   64

// RGB565 colors
#ifndef RED
  #define RED   0xF800
#endif
#ifndef GREEN
  #define GREEN 0x07E0
#endif
#ifndef BLACK
  #define BLACK 0x0000
#endif
#ifndef WHITE
  #define WHITE 0xFFFF
#endif

// Pass this struct into step_counter_task
// (unified handle + fonts)
typedef struct {
    TFT_t     *dev;
    FontxFile *fxBig;
    FontxFile *fxSmall;
} display_args_t;

/* OLD PLACEHOLDER
// Icon placeholder
extern const uint8_t foot_icon_bits[];
extern const uint8_t x_icon_bits[];
extern const uint8_t fire_icon_bits[];

// Dog sprites placeholder
extern const uint8_t *dog_sprite_bits[6];  // index with movement_state_t
*/

typedef enum {
    MOOD_DEAD = 0,
    MOOD_EXCITED,
    MOOD_HAPPY,
    MOOD_NEUTRAL,
    MOOD_SAD,
    MOOD_COUNT
} movement_mood_t;

void drawMoveMate(
    TFT_t             *dev,
    FontxFile         *fxBig,
    FontxFile         *fxSmall,
    int                steps,
    int                deaths,
    int                streak,
    int                score,            // 0…10000
    movement_state_t   state,
    movement_mood_t    mood,
    bool               stationary
);

#endif // DISPLAY_H