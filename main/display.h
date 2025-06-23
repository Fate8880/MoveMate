// display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "st7789.h"
#include "fontx.h"
#include "stepcounter.h"

// Argument struct
typedef struct {
    TFT_t     *dev;
    FontxFile *fxBig;
    FontxFile *fxSmall;
} display_args_t;

// Draw Method for Main Screen
void drawMoveMate(
    TFT_t          *dev,
    FontxFile      *fxBig,
    FontxFile      *fxSmall,
    int             steps,
    int             deaths,
    int             streak,
    int             score,
    movement_state_t state
);

#endif // DISPLAY_H
