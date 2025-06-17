// stepcounter.c
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "st7789.h"
#include "sensor.h"    

#define BP_B0  0.010432f
#define BP_B1  0.000000f
#define BP_B2  -0.020865f
#define BP_A1  -3.676376f
#define BP_A2  5.087648f

// === Tuning parameters ===
#define THRESHOLD            0.08f      // ADJUST
#define MIN_STEP_INTERVAL_US 300000     // 300 ms
#define SAMPLE_PERIOD_US     10000      // 10 ms → 100 Hz

static const char *TAG = "STEP_COUNTER";


static BiquadState bp = {0};


// Argument struct (same as before)
typedef struct {
    TFT_t     *dev;
    FontxFile *fx;
} step_args_t;

void step_counter_task(void *pvParameters)
{
    step_args_t *args = pvParameters;
    TFT_t     *dev = args->dev;
    FontxFile *fx  = args->fx;

    // Display init
    lcdSetFontDirection(dev, DIRECTION90);
    lcdFillScreen(dev, BLACK);
    lcdDrawFinish(dev);
    const int X = 10, Y = 10;

    // Buffer samples for real-time local-max test
    float prev = 0.0f, curr = 0.0f, next = 0.0f;
    int64_t last_step_us = 0;
    int    step_count   = 0;

    // Prime the filter with one sample
    {
        float ax, ay, az, gx, gy, gz;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        float mag = sqrtf(ax*ax + ay*ay + az*az);
        curr = biquad_bandpass(mag, &bp);
    }

    while (1) {
        // shift window
        prev = curr;
        curr = next;

        // read raw IMU
        float ax, ay, az, gx, gy, gz;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        float mag = sqrtf(ax*ax + ay*ay + az*az);

        // band-pass filter
        next = biquad_bandpass(mag, &bp);

        // detect local maximum above threshold
        if (curr > prev && curr > next && curr > THRESHOLD) {
            int64_t now = esp_timer_get_time();
            if (now - last_step_us > MIN_STEP_INTERVAL_US) {
                last_step_us = now;
                step_count++;
                ESP_LOGI(TAG, "Step %d @ %.2fs  mag=%.2f",
                         step_count, now/1e6f, curr);

                // ==== update display ====
                lcdFillScreen(dev, BLACK);
                char bufstr[20];
                snprintf(bufstr, sizeof(bufstr), "Steps:%4d", step_count);
                lcdDrawString(dev, fx, X, Y, (uint8_t*)bufstr, WHITE);
                lcdDrawFinish(dev);
            }
        }

        // wait for next sample
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_US / 1000));
    }
}
