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
#include "stepcounter.h"
#include "display.h"

// === Biquad filter coefficients ===
/*
#define BP_B0  0.010432f
#define BP_B1  0.000000f
#define BP_B2  -0.020865f
#define BP_A1  -3.676376f
#define BP_A2  5.087648f
*/

// === Tuning parameters ===
#define STEP_CONFIRMATION_WINDOW   1200000  // 1.2 seconds
#define MIN_STEP_INTERVAL_US       300000   // 300 ms
#define SAMPLE_PERIOD_US           10000    // 10 ms → 100 Hz
#define THRESHOLD                  0.08f    // ADJUST
#define THRESHOLD_WEAK             1.5f
#define THRESHOLD_STRONG           3.0f

static const char *TAG = "STEP_COUNTER";
static BiquadState  bp  = { .x1 = 0, .x2 = 0, .y1 = 0, .y2 = 0 };

void step_counter_task(void *pvParameters) {
    // Display
    display_args_t *args = (display_args_t*)pvParameters;
    TFT_t     *dev     = args->dev;
    FontxFile *fxBig   = args->fxBig;
    FontxFile *fxSmall = args->fxSmall;

    // Rotate our text for portrait mode, clear once
    lcdSetFontDirection(dev, DIRECTION90);
    lcdFillScreen(dev, BLACK);
    lcdDrawFinish(dev);

    // Counters and state
    int               step_count      = 0;
    int               deaths          = 0;
    int               streak          = 0;
    int               score           = 0;  // 0 - 10000??, just a guess
    movement_state_t  state           = STATE_IDLE;
    float             weak_duration   = 0.0f;
    float             strong_duration = 0.0f;

    // Timing for display and step intervals
    static int64_t    last_display_us  = 0;
    int64_t           last_step_us     = 0;
    int64_t           state_entry_time = 0;
    int64_t           first_step_time  = 0;

    // Three‐point window for local‐max detection
    float prev = 0.0f, curr = 0.0f, next = 0.0f;

    // Prime the filter with one sample
    {
        float ax, ay, az, gx, gy, gz;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        float mag = sqrtf(ax*ax + ay*ay + az*az);
        curr = biquad_bandpass(mag, &bp);
    }

    while (1) {
        // Shift window for step detection
        prev = curr;
        curr = next;

        // Read raw IMU and Band-pass filter
        float ax, ay, az, gx, gy, gz;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        float mag = sqrtf(ax*ax + ay*ay + az*az);
        next = biquad_bandpass(mag, &bp);

        int64_t now = esp_timer_get_time();

        // STATE DETECTION

        // IDLE
        if (state == STATE_WALKING && now - state_entry_time > 1500000) {
            state = STATE_IDLE;
        } else if ((state == STATE_WEAK || state == STATE_STRONG)
                   && now - state_entry_time > 500000) {
            state = STATE_IDLE;
        }

        // STEP DETECTOR
        if (curr > prev && curr > next && curr > THRESHOLD) {
            if (state==STATE_IDLE || state==STATE_WEAK || state==STATE_STRONG) {
                state = STATE_POTENTIAL_STEP;
                state_entry_time = now;
                first_step_time  = now;
                ESP_LOGI(TAG, "Potential step detected, waiting for confirmation");
            }
            else if (state==STATE_POTENTIAL_STEP
                     && (now - first_step_time) > 500000) {
                if ((now - first_step_time) <= STEP_CONFIRMATION_WINDOW) {
                    step_count += 2;
                    state = STATE_WALKING;
                    ESP_LOGI(TAG, "Walking confirmed - counted 2 steps");
                } else {
                    // first step was most likely a strong movement
                    state = STATE_STRONG;
                    strong_duration += 1.0f; // CHANGE
                    ESP_LOGI(TAG, "Strong movement (timeout)");
                }
                state_entry_time = now;
            }
            else if (state==STATE_WALKING && (now - last_step_us) > MIN_STEP_INTERVAL_US) { // continue walking
                last_step_us = now;
                step_count++;
                state = STATE_WALKING;
                state_entry_time = now;
                ESP_LOGI(TAG, "Step %d @ %.2fs  mag=%.2f", step_count, now/1e6f, curr);
            }
        }

        // NON-PERIODIC MOVEMENTS
        else if ((state==STATE_IDLE || state==STATE_WEAK || state==STATE_STRONG) && mag > THRESHOLD_WEAK) {
            if (mag > THRESHOLD_STRONG) {
                state = STATE_STRONG;
                strong_duration += 0.1f;
                ESP_LOGI(TAG, "Strong movement: %.2f, mag=%.2f", strong_duration, mag);
            } else {
                state = STATE_WEAK;
                weak_duration += 0.1f;
                ESP_LOGI(TAG, "Weak movement: %.2f, mag=%.2f", weak_duration, mag);
            }
            state_entry_time = now;
        }

        // TOO LONG IN POTENTIAL STEP
        if (state==STATE_POTENTIAL_STEP && (now - state_entry_time) > STEP_CONFIRMATION_WINDOW) {
            //first step was most likely a strong movement
            state = STATE_STRONG;
            strong_duration += 1.0f;  // CHANGE
            state_entry_time = now;
            ESP_LOGI(TAG, "Step confirmation timeout → STRONG");
        }

        // TODO: Update deaths, streak, score

        // Display
        if ((now - last_display_us) > 500000) {
            last_display_us = now;
            drawMoveMate(
                dev, fxBig, fxSmall,
                step_count,
                deaths,
                streak,
                score,
                state
            );
        }

        // wait for next sample
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_US / 1000));
    }
}
