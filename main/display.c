#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "display.h"
#include "icons.h"

// Bitmap Drawer
static void lcdDrawBitmap(
    TFT_t        *dev,
    const uint8_t bits[],
    int           w,
    int           h,
    int           x0,
    int           y0,
    uint16_t      fg_color
){
    int bytesPerRow = (w + 7) >> 3;
    for(int row = 0; row < h; row++){
        for(int col = 0; col < w; col++){
            int byteIndex = row * bytesPerRow + (col >> 3);
            uint8_t mask  = 0x80 >> (col & 7);
            if(bits[byteIndex] & mask) {
                lcdDrawPixel(dev, x0 + col, y0 + row, fg_color);
            }
        }
    }
}

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
static inline void drawBMP(TFT_t *dev, const uint8_t *b, int w, int h, int x, int y, uint16_t c) {
    // pass swapped
    lcdDrawBitmap(dev, b, w, h, y, x, c);
}
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

    // our four colour-stops in RGB565
    const uint16_t stops[4] = {
        0xF800, // deep red
        0xFAA0, // deep orange
        0xFFE0, // deep yellow
        0x07E0  // deep green
    };

    int pos = 0;

    // 1) LEFT edge: top→bottom
    for(int dy = 0; dy < h && pos < filled; dy++, pos++) {
        // compute colour
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

    // 2) BOTTOM edge: left→right at y + h - 1
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

    // 3) RIGHT edge: bottom→top
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


void drawMoveMate(
    TFT_t          *dev,
    FontxFile      *fxBig,
    FontxFile      *fxSmall,
    int             steps,
    int             deaths,
    int             streak,
    int             score,
    movement_state_t state
) {
    const int TOTAL_W = 135, TOTAL_H = 240;
    // Font Direction is 90, so swap width and height
    const int W = TOTAL_H;
    const int H = TOTAL_W;

    const int PAD = 10;
    // Split screen: left stats, right panel
    int splitX = W / 2;

    lcdFillScreen(dev, BLACK);

    //Left: stats with sprites vertically aligned
    const char *labels[] = { NULL, NULL, NULL };
    int values[3] = { steps, deaths, streak };
    const uint8_t *icons[3] = { foot_icon_bits, x_icon_bits, fire_icon_bits };
    const uint16_t *icons565[3] = { foot_icon_pixels, x_icon_pixels, fire_icon_pixels };

    int rowH = (H - 2*PAD) / 3;
    for(int i=0; i<3; i++){
        int row = 2 - i;

        char buf[16];
        sprintf(buf, "%d", values[i]);
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
        //drawBMP(dev, icons[i], ICON_W, ICON_H, iconX, y, GREEN);
        drawBMP565(dev, icons565[i], ICON_W, ICON_H, iconX, y);
    }

    // Right: score, sprite, state
    int panelX = splitX + PAD;
    int panelW = W - panelX - PAD;
    int panelY = PAD;
    int panelH = H - 2*PAD;

    // Draw Score Progressbar
    drawGradientBorder(dev, panelX, panelY, panelW, panelH, score);

    // State text uppercase
    char tbuf[16];
    const char *names[] = {"WALKING","WEAK","STRONG","IDLE","STEP?", "RUNNING"};
    strcpy(tbuf, names[state]);
    int tw = strlen(tbuf) * getFortWidth(&fxSmall[0]);
    int tx = panelX + (panelW - tw)/2;
    int ty = panelY;
    drawSTR(dev, fxSmall, tx, ty, (uint8_t*)tbuf, WHITE);

    // Dog sprite (64×64)
    int imgX = panelX + (panelW - SPRITE_W)/2;
    int imgY = panelY + getFortHeight(&fxBig[0]);
    drawBMP(dev, dog_sprite_bits[state], SPRITE_W, SPRITE_H,
                  imgX, imgY, WHITE);

    // Score
    char sbuf[16];
    sprintf(sbuf, "%d", score);
    int sw = strlen(sbuf) * getFortWidth(&fxBig[0]);
    int sx = panelX + (panelW - sw)/2;
    int sy = imgY + SPRITE_H;
    drawSTR(dev, fxBig, sx, sy, (uint8_t*)sbuf, WHITE);

    lcdDrawFinish(dev);
}
