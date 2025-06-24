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
#include "driver/gpio.h"

bool testDisplay = false; // Is for debugging

// === Tuning parameters ===
#define STEP_CONFIRMATION_WINDOW    1200000  // 1.2 seconds
#define MIN_STEP_INTERVAL_US        300000   // 300 ms
#define SAMPLE_PERIOD_US            10000    // 10 ms → 100 Hz
#define THRESHOLD                   0.35f    // ADJUST 0.3-0.37
#define THRESHOLD_WEAK              1.3f
#define THRESHOLD_STRONG            2.0f
// RUNNING
#define STEP_FREQ_WINDOW            5
#define THRESHOLD_RUNNING           2.2f

#define BUTTON_A                    37         // Button at the front

static uint64_t step_times[STEP_FREQ_WINDOW] = {0};
static int step_time_idx = 0;

static const char *TAG = "STEP_COUNTER";
static BiquadState  bp  = { .x1 = 0, .x2 = 0, .y1 = 0, .y2 = 0 };

// Walking vs Running
void update_step_frequency(int64_t now, movement_state_t *state) {
    step_times[step_time_idx % STEP_FREQ_WINDOW] = now;
    step_time_idx++;

    ESP_LOGI(TAG, "Step_time_idx: %d", step_time_idx);

    if (step_time_idx >= STEP_FREQ_WINDOW) {
        int oldest_idx = (step_time_idx - STEP_FREQ_WINDOW) % STEP_FREQ_WINDOW;
        int newest_idx = (step_time_idx - 1) % STEP_FREQ_WINDOW;

        float duration_s = (step_times[newest_idx] - step_times[oldest_idx]) / 1e6f;
        float frequency_hz = (STEP_FREQ_WINDOW - 1) / duration_s;
        ESP_LOGI(TAG, "Step frequency: %.2f Hz", frequency_hz);

        if (frequency_hz > THRESHOLD_RUNNING) {
            if (*state != STATE_RUNNING) {
                *state = STATE_RUNNING;
                ESP_LOGI(TAG, "Running detected: %.2f Hz", frequency_hz);
            }
        } else {
            if (*state != STATE_WALKING) {
                *state = STATE_WALKING;
                ESP_LOGI(TAG, "Walking detected: %.2f Hz", frequency_hz);
            }
        }
    }
}

void init_button() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_A),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}


void step_counter_task(void *pvParameters) {
    // Display
    display_args_t *args = (display_args_t*)pvParameters;
    TFT_t     *dev     = args->dev;
    FontxFile *fxBig   = args->fxBig;
    FontxFile *fxSmall = args->fxSmall;

    // Button
    init_button();
    bool stationary_mode = false;
    int last_button_state = 1; // pulled-up, not pressed

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
    movement_mood_t   mood            = MOOD_NEUTRAL;

    // Timing for display and step intervals
    int64_t           last_display_us  = esp_timer_get_time();
    int64_t           last_sim_us      = last_display_us;
    static int        sim_step         = 0;
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
        int64_t now = esp_timer_get_time();

        // Button pressed?
        int current_button_state = gpio_get_level(BUTTON_A);
        if (!current_button_state && last_button_state) {
            if (stationary_mode) {
                stationary_mode = false;
                ESP_LOGI(TAG, "Stationary mode OFF");
            } else {
                stationary_mode = true;
                ESP_LOGI(TAG, "Stationary mode ON");
            }
        }
        last_button_state = current_button_state;

        if (testDisplay) {
            static int sim_step = 0;
            int64_t now = esp_timer_get_time();

            if (now - last_sim_us >= 1000000) {
                last_sim_us = now;
                sim_step    = (sim_step + 1) % 10;  // 0…9
                stationary_mode = !stationary_mode;

                if (sim_step < 5) {
                    // 0–4: moods
                    state = STATE_IDLE;
                    mood  = (movement_mood_t)sim_step;
                }
                else if (sim_step < 9) {
                    static const movement_state_t seq[4] = {
                        STATE_WALKING,
                        STATE_RUNNING,
                        STATE_WEAK,
                        STATE_STRONG
                    };
                    state = seq[sim_step - 5];
                    mood  = MOOD_NEUTRAL;
                }
                else {
                    state = STATE_POTENTIAL_STEP;
                    mood  = (movement_mood_t)(-1);
                }

                // advance stats so you can see the border move
                step_count++;
                score = (score + 250 > 10000) ? 10000 : score + 250;
                if (!(step_count % 5))  streak++;
                if (!(step_count % 40)) {
                    deaths++;
                    step_count = streak = score = 0;
                }
            }
        } else {
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
            } else if ((state == STATE_WEAK || state == STATE_STRONG) && now - state_entry_time > 500000) {
                state = STATE_IDLE;
            }

            // STEP DETECTOR
            if (curr > prev && curr > next && curr > THRESHOLD && !stationary_mode) {
                if (state==STATE_IDLE || state==STATE_WEAK || state==STATE_STRONG) {
                    state = STATE_POTENTIAL_STEP;
                    state_entry_time = now;
                    first_step_time  = now;
                    ESP_LOGI(TAG, "Potential step detected, waiting for confirmation");
                }
                else if (state==STATE_POTENTIAL_STEP && (now - first_step_time) > 500000) {
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
                else if ((state==STATE_WALKING || state==STATE_RUNNING) && (now - last_step_us) > MIN_STEP_INTERVAL_US) { // continue walking
                    last_step_us = now;
                    step_count++;
                    update_step_frequency(now, &state);
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
        }

            // TOO LONG IN POTENTIAL STEP
            if (state==STATE_POTENTIAL_STEP && (now - state_entry_time) > STEP_CONFIRMATION_WINDOW) {
            //first step was most likely a strong movement
            state = STATE_STRONG;
            strong_duration += 1.0f;  // TODO: CHANGE
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
                state,
                mood,
                stationary_mode
            );
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_US / 1000));
    }
}
