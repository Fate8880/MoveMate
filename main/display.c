#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "display.h"
#include "icons.h"

static void lcdDrawBitmapRGB565(
    TFT_t   *dev,
    const uint16_t pixels[],
    int       w,
    int       h,
    int       x0,
    int       y0
){
    for(int row=0; row<h; row++){
        for(int col=0; col<w; col++){
            uint16_t c = pixels[row * w + col];
            lcdDrawPixel(dev, x0 + col, y0 + row, c);
        }
    }
}


// Helper Methods, rotations, and what is up and down, are confusing otherwise
static inline void drawPX(TFT_t *dev, int x, int y, uint16_t c) {
    // swap X/Y when addressing the hardware
    lcdDrawPixel(dev, y, x, c);
}
/* OLD BITMAP DRAWER
static inline void drawBMP(TFT_t *dev, const uint8_t *b, int w, int h, int x, int y, uint16_t c) {
    // pass swapped
    lcdDrawBitmap(dev, b, w, h, y, x, c);
}
*/
static inline void drawBMP565(TFT_t *dev, const uint16_t *px, int w, int h, int x, int y) {
    // swap X/Y if you still need that
    lcdDrawBitmapRGB565(dev, px, w, h, y, x);
}
static inline void drawSTR(TFT_t *dev, FontxFile *f, int x, int y, uint8_t *s, uint16_t c) {
    // pass swapped
    lcdDrawString(dev, f, y, x, s, c);
}

// Color Interpolation
static uint16_t lerpColor(uint16_t c0, uint16_t c1, float t) {
    uint8_t r0 = (c0 >> 11) & 0x1F;
    uint8_t g0 = (c0 >> 5)  & 0x3F;
    uint8_t b0 =  c0        & 0x1F;
    uint8_t r1 = (c1 >> 11) & 0x1F;
    uint8_t g1 = (c1 >> 5)  & 0x3F;
    uint8_t b1 =  c1        & 0x1F;
    uint8_t r = r0 + (int)((r1 - r0)*t);
    uint8_t g = g0 + (int)((g1 - g0)*t);
    uint8_t b = b0 + (int)((b1 - b0)*t);
    return (r<<11)|(g<<5)|b;
}

// Score Progress Bar
static void drawGradientBorder(
    TFT_t *dev, int x, int y, int w, int h, int score
) {
    const int TH = 2;
    const int MAX_SCORE = 10000;
    int perim = h + w + h;
    int filled = (score >= MAX_SCORE)
                   ? perim
                   : (score * perim) / MAX_SCORE;

    // Color Def for Border
    const uint16_t stops[4] = {
        0xF800, // red
        0xFAA0, // orange
        0xFFE0, // yellow
        0x07E0  // green
    };

    int pos = 0;

    // LEFT edge
    for(int dy = 0; dy < h && pos < filled; dy++, pos++) {
        float tn  = (float)pos / (perim - 1);
        float ft  = tn * 3.0f;
        int   idx = (int)ft;
        if (idx > 2) idx = 2;
        float local_t = ft - idx;
        uint16_t c = lerpColor(stops[idx], stops[idx+1], local_t);

        for(int tx = 0; tx < TH; tx++) {
            drawPX(dev,
                   x + tx,
                   y + dy,
                   c);
        }
    }

    // TOP edge
    for(int dx = 0; dx < w && pos < filled; dx++, pos++) {
        float tn  = (float)pos / (perim - 1);
        float ft  = tn * 3.0f;
        int   idx = (int)ft;
        if (idx > 2) idx = 2;
        float local_t = ft - idx;
        uint16_t c = lerpColor(stops[idx], stops[idx+1], local_t);

        for(int ty = 0; ty < TH; ty++) {
            drawPX(dev,
                   x + dx,
                   y + (h - 1 - ty),
                   c);
        }
    }

    // RIGHT edge
    for(int dy = 0; dy < h && pos < filled; dy++, pos++) {
        float tn  = (float)pos / (perim - 1);
        float ft  = tn * 3.0f;
        int   idx = (int)ft;
        if (idx > 2) idx = 2;
        float local_t = ft - idx;
        uint16_t c = lerpColor(stops[idx], stops[idx+1], local_t);

        for(int tx = 0; tx < TH; tx++) {
            drawPX(dev,
                   x + (w - 1 - tx),
                   y + (h - 1 - dy),
                   c);
        }
    }
}

static void drawFullBorder(TFT_t *dev, int W, int H, int TH, uint16_t c) {
    // top + bottom
    for (int t = 0; t < TH; t++) {
        for (int x = 0; x < W; x++) {
            drawPX(dev, x, t,     c);
            drawPX(dev, x, H-1-t, c);
        }
    }
    // left + right
    for (int t = 0; t < TH; t++) {
        for (int y = 0; y < H; y++) {
            drawPX(dev, t,     y, c);
            drawPX(dev, W-1-t, y, c);
        }
    }
}

void drawMoveMate(
    TFT_t           *dev,
    FontxFile       *fxBig,
    FontxFile       *fxSmall,
    int              steps,
    int              deaths,
    int              streak,
    int              score,
    movement_state_t state,
    movement_mood_t  mood,
    bool             stationary
) {
    const int TOTAL_W = 135, TOTAL_H = 240;
    // Font Direction is 90, so swap width and height
    const int W = TOTAL_H;
    const int H = TOTAL_W;

    const int PAD = 10;
    // Split screen: left stats, right panel
    int splitX = W / 2;

    lcdFillScreen(dev, BLACK);

    // Left: stats with sprites vertically aligned
    { // Bracket for easier reading
        int values[3] = { steps, deaths, streak };
        const uint16_t *icons565[3] = {
            foot_icon_pixels, 
            x_icon_pixels, 
            fire_icon_pixels 
        };

        int rowH = (H - 2*PAD) / 3;
        for(int i=0; i<3; i++) {
            int row = 2 - i;

            char buf[16];
            bool isStepRow = (i == 0);
            if (stationary && i == 0) {
                strcpy(buf, "STNRY");
            } else {
                sprintf(buf, "%d", values[i]);
            }

            int txtW  = strlen(buf) * getFortWidth(&fxBig[0]);

            int iconX = splitX - PAD - ICON_W;
            int textX = iconX - 4 - txtW;

            int baseY = PAD + row*rowH;
            int y     = baseY + (rowH - ICON_H)/2;

            // Stat number
            drawSTR(dev, fxBig,
                    textX,
                    y + (ICON_H - getFortHeight(&fxBig[0]))/2,
                    (uint8_t*)buf, WHITE);
            
            // Stat Sprite (32x32)
            drawBMP565(dev, icons565[i], ICON_W, ICON_H, iconX, y);
        }
    }
    
    // Right: score, sprite, state
    { // Bracket for easier reading
        int panelX = splitX + PAD;
        int panelW = W - panelX - PAD;
        int panelY = PAD;
        int panelH = H - 2*PAD;

        // Draw Score Progressbar
        drawGradientBorder(dev, panelX, panelY, panelW, panelH, score);

        if (stationary) {
            drawFullBorder(dev, W, H, 5, 0xFFE0);
        }

        // Decide Label and Sprite, then store
        const char *label;
        const uint16_t *sprite_pixels;

        if (state < 0 || state >= STATE_COUNT) {
            state = STATE_IDLE;
        }

        if (state != STATE_IDLE) {
            switch(state) {
              case STATE_WALKING:
                label = "WALKING";
                sprite_pixels = walking_icon_pixels;
                break;
              case STATE_WEAK:
                label = "WEAK";
                sprite_pixels = weak_icon_pixels;
                break;
              case STATE_STRONG:
                label = "STRONG";
                sprite_pixels = strong_icon_pixels;
                break;
              case STATE_RUNNING:
                label = "RUNNING";
                sprite_pixels = running_icon_pixels;
                break;
              default:
                // any other invalid state
                label = "IDLE";
                sprite_pixels = neutral_icon_pixels;
                break;
            }
        } else {
            // idle → show mood
            static const char *moodNames[] = {
                "DEAD", "EXCITED", "HAPPY", "NEUTRAL", "SAD"
            };
            static const uint16_t *mood_pixels[] = {
                dead_icon_pixels,
                excited_icon_pixels,
                happy_icon_pixels,
                neutral_icon_pixels,
                sad_icon_pixels
            };
            int m = (mood >= 0 && mood < MOOD_COUNT) ? (int)mood : MOOD_NEUTRAL;
            label = moodNames[m];
            sprite_pixels = mood_pixels[m];
        }


        // State text uppercase
        int tw = strlen(label) * getFortWidth(&fxSmall[0]);
        int tx = panelX + (panelW - tw)/2;
        int ty = panelY;
        drawSTR(dev, fxSmall, tx, ty, (uint8_t*)label, WHITE);

        // Dog sprite (64×64)
        int imgX = panelX + (panelW - SPRITE_W)/2;
        int imgY = panelY + getFortHeight(&fxBig[0]);
        drawBMP565(dev, sprite_pixels, SPRITE_W, SPRITE_H, imgX, imgY);

        // Score
        char sbuf[16];
        sprintf(sbuf, "%d", score);
        int sw = strlen(sbuf) * getFortWidth(&fxBig[0]);
        int sx = panelX + (panelW - sw)/2;
        int sy = imgY + SPRITE_H;
        drawSTR(dev, fxBig, sx, sy, (uint8_t*)sbuf, WHITE);
    }
    

    lcdDrawFinish(dev);
}
