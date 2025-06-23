// display.c
#include <stdio.h>
#include "display.h"

void drawMoveMate(
    TFT_t          *dev,
    FontxFile      *fxBig,
    FontxFile      *fxSmall,
    int             steps,
    int             deaths,
    int             streak,
    int             score,
    movement_state_t state
){
    const int X = 10, Y = 10;
    char steps_buf[32], state_buf[32];
    const char *state_str;

    switch(state){
      case STATE_WALKING:       state_str = "Walking";       break;
      case STATE_WEAK:          state_str = "Weak";          break;
      case STATE_STRONG:        state_str = "Strong";        break;
      case STATE_IDLE:          state_str = "Idle";          break;
      case STATE_POTENTIAL_STEP:state_str = "Step?";         break;
      default:                  state_str = "Unknown";       break;
    }

    lcdFillScreen(dev, BLACK);
    snprintf(steps_buf, sizeof(steps_buf), "Steps:%4d", steps);
    snprintf(state_buf, sizeof(state_buf), "State: %s", state_str);

    lcdDrawString(dev, fxBig, X, Y, (uint8_t*)steps_buf, WHITE);
    lcdDrawString(dev, fxBig, X + 50, Y, (uint8_t*)state_buf, WHITE);
    lcdDrawFinish(dev);
}
