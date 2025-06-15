#pragma once

#include "esp_err.h"

//void accel_sampling_task(void *pvParameters);
esp_err_t sensor_i2c_master_init(void);
void init_mpu6886(void);
void getAccelData(float *ax, float *ay, float *az);
void motion_sampling_task(void *pvParameters);
void get_mpu_readings(float *ax, float *ay, float *az, float *gx, float *gy, float *gz);
esp_err_t init_accel_filters(float sample_rate, float hp_cutoff, float lp_cutoff);
void filter_accel_mag(float ax, float ay, float az, float *filtered_mag);

#ifndef FILTER_H
#define FILTER_H

#include <math.h>

// Biquad filter state (Direct Form I)
typedef struct {
    float x1, x2;  // previous inputs
    float y1, y2;  // previous outputs
} BiquadState;

// Bandpass filter coefficients
#define BP_B0  0.015621f
#define BP_B1  0.000000f
#define BP_B2 -0.015621f
#define BP_A1 -1.734726f
#define BP_A2  0.766006f

// Band-pass biquad filter function
float biquad_bandpass(float x, BiquadState* state);

#endif