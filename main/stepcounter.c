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
// STEPS
#define THRESHOLD            0.08f      // ADJUST
#define MIN_STEP_INTERVAL_US 300000     // 300 ms
#define SAMPLE_PERIOD_US     10000      // 10 ms → 100 Hz
// MOVEMENTS
#define THRESHOLD_WEAK 1.5f
#define THRESHOLD_STRONG 3.0f

static const char *TAG = "STEP_COUNTER";
static BiquadState bp = {0};

// Argument struct
typedef struct {
    TFT_t     *dev;
    FontxFile *fx;
} step_args_t;

// States
typedef enum {
    STATE_WALKING,
    STATE_WEAK,
    STATE_STRONG,
    STATE_IDLE
} movement_state_t;

void step_counter_task(void *pvParameters) {
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

    float weak_duration = 0;
    float strong_duration = 0;

    // Movement state tracking
    movement_state_t state = STATE_IDLE;
    int64_t state_entry_time = 0; // Time when the state was last changed

    // Prime the step filter with one sample
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

        // Read raw IMU
        float ax, ay, az, gx, gy, gz;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        float mag = sqrtf(ax*ax + ay*ay + az*az);

        // Band-pass filter
        next = biquad_bandpass(mag, &bp);
        int64_t now = esp_timer_get_time();
        

        // STATE DETECTION

        // IDLE
        if (state == STATE_WALKING) {
            if (now - state_entry_time > 1000000) {
                state = STATE_IDLE;
            }
        } else if (state == STATE_WEAK || state == STATE_STRONG) {
                if (now - state_entry_time > 500000) {
                state = STATE_IDLE;
            }
        }

        // STEP DETECTOR
        if ((state == STATE_IDLE || state == STATE_WALKING || state == STATE_STRONG || state == STATE_WEAK) && (curr > prev && curr > next && curr > THRESHOLD)) { // detect local maximum above threshold
            int64_t now = esp_timer_get_time();
            if (now - last_step_us > MIN_STEP_INTERVAL_US) {
                last_step_us = now;
                step_count++;
                state = STATE_WALKING;
                state_entry_time = now;
                ESP_LOGI(TAG, "Step %d @ %.2fs  mag=%.2f", step_count, now/1e6f, curr);
            }
          
        // NON-PERIODIC MOVEMENTS
        } else if ((state == STATE_IDLE || state == STATE_STRONG || state == STATE_WEAK) && mag > THRESHOLD_WEAK) {
            if (mag > THRESHOLD_STRONG) {
                state = STATE_STRONG;
                strong_duration += 0.1;
                state_entry_time = now;
                ESP_LOGI(TAG, "Strong movement: %.2f, mag=%.2f", strong_duration, mag);
            } else {
                state = STATE_WEAK;
                weak_duration += 0.1;
                state_entry_time = now;
                ESP_LOGI(TAG, "Weak movement: %.2f, mag=%.2f", weak_duration, mag);
            }
        }

        // state to string
        char* state_str;
        switch (state) {
            case STATE_WALKING: state_str = "Walking"; break;
            case STATE_WEAK: state_str = "Weak"; break;
            case STATE_STRONG: state_str = "Strong"; break;
            case STATE_IDLE: state_str = "Idle"; break;
            default: state_str = "Unknown"; break;
        }

        // Display
        lcdFillScreen(dev, BLACK);
        char steps_buf[20], weak_buf[20], strong_buf[20], state_buf[20];
        snprintf(steps_buf, sizeof(steps_buf), "Steps:%4d", step_count);
        snprintf(weak_buf, sizeof(weak_buf), "Weak:%.2f s", weak_duration);
        snprintf(strong_buf, sizeof(strong_buf), "Strong:%.2f s", strong_duration);
        snprintf(state_buf, sizeof(state_buf), "State: %s", state_str);

        lcdDrawString(dev, fx, X, Y, (uint8_t*)steps_buf, WHITE);
        lcdDrawString(dev, fx, X + 50, Y, (uint8_t*)state_buf, WHITE);
        lcdDrawFinish(dev);    

        ESP_LOGI(TAG, "State: %s", state_str);
        
        // wait for next sample
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_US / 1000));
    }
}
