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