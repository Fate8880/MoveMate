#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "st7789.h"
#include "sensor.h"

#define STEP_THRESHOLD  0.18f  

static const char *TAG = "STEP_COUNTER";

typedef struct {
    TFT_t     *dev;
    FontxFile *fx;
} step_args_t;


void step_counter_task(void *pvParameters)
{
    step_args_t *args = pvParameters;
    TFT_t     *dev = args->dev;
    FontxFile *fx  = args->fx;

    float prev_filt = 0.0f;
    int   step_count = 0;

    // Display
    lcdSetFontDirection(dev, DIRECTION90);
    lcdFillScreen(dev, BLACK);
    lcdDrawFinish(dev);
    int X = 10, Y = 10;

    while (1) {
        // Read IMU data
        float ax, ay, az, gx, gy, gz, filt;
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        filter_accel_mag(ax, ay, az, &filt);

        // Step detection based on filtered acceleration threshold
         if (filt > STEP_THRESHOLD && prev_filt <= STEP_THRESHOLD) {
            step_count++;
            ESP_LOGI(TAG, "Step %d", step_count);
        }
        prev_filt = filt;

        // Update display
        static int lastDispCount = -1;
        if (step_count != lastDispCount) {
            lcdFillScreen(dev, BLACK);

            char buf[16];
            snprintf(buf, sizeof(buf), "Steps:%4d", step_count);
            lcdDrawString(dev, fx, X, Y, (uint8_t*)buf, WHITE);

            lcdDrawFinish(dev);

            lastDispCount = step_count;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}