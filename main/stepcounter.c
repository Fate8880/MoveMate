#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "st7789.h"
#include "sensor.h"
#include "fontx.h"  // Needed for FontxFile definition

// Configuration
#define SAMPLE_RATE_HZ     100
#define HISTORY_SIZE       5
#define MIN_PEAK_HEIGHT    0.2f
#define MIN_STEP_INTERVAL  250
#define PEAK_WINDOW        2

// Peak detector structure
typedef struct {
    float values[HISTORY_SIZE];
    int index;
} PeakDetector;

// Task arguments structure (must match what's passed from main.c)
typedef struct {
    TFT_t *dev;
    FontxFile *fx;
} step_counter_args_t;

static void init_detector(PeakDetector *pd) {
    memset(pd->values, 0, sizeof(pd->values));
    pd->index = 0;
}

static void push_sample(PeakDetector *pd, float value) {
    pd->values[pd->index] = value;
    pd->index = (pd->index + 1) % HISTORY_SIZE;
}

static bool is_peak(PeakDetector *pd) {
    int center_pos = (pd->index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    float center = pd->values[center_pos];
    
    if (center < MIN_PEAK_HEIGHT) {
        return false;
    }

    for (int i = 1; i <= PEAK_WINDOW; i++) {
        int prev_pos = (center_pos - i + HISTORY_SIZE) % HISTORY_SIZE;
        int next_pos = (center_pos + i) % HISTORY_SIZE;
        
        if (pd->values[prev_pos] > center || pd->values[next_pos] > center) {
            return false;
        }
    }
    return true;
}

void step_counter_task(void *pvParameters) {
    step_counter_args_t *args = (step_counter_args_t *)pvParameters;
    TFT_t *dev = args->dev;
    FontxFile *fx = args->fx;

    PeakDetector detector;
    init_detector(&detector);
    
    int step_count = 0;
    int64_t last_step_time = 0;
    char bufstr[20];

    // Display initialization
    lcdSetFontDirection(dev, DIRECTION90);
    lcdFillScreen(dev, BLACK);
    lcdDrawString(dev, fx, 10, 10, (uint8_t*)"Steps: 0", WHITE);
    lcdDrawFinish(dev);

    while (1) {
        // Read sensor data
        float ax, ay, az, gx, gy, gz, filt_mag;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        filter_accel_mag(ax, ay, az, &filt_mag);

        // Update detector
        push_sample(&detector, filt_mag);

        // Check for step
        int64_t now = esp_timer_get_time();
        if (is_peak(&detector)) {
            if ((now - last_step_time) > (MIN_STEP_INTERVAL * 1000)) {
                step_count++;
                last_step_time = now;
                
                // Update display
                snprintf(bufstr, sizeof(bufstr), "Steps: %d", step_count);
                lcdFillScreen(dev, BLACK);
                lcdDrawString(dev, fx, 10, 10, (uint8_t*)bufstr, WHITE);
                lcdDrawFinish(dev);
                
                ESP_LOGI("STEP_COUNTER", "Step %d (%.2fg)", step_count, 
                        detector.values[(detector.index-1+HISTORY_SIZE)%HISTORY_SIZE]);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000/SAMPLE_RATE_HZ));
    }
}