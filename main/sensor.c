//sensor.c
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "sensor.h"
#include <math.h>

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define MPU6886_SENSOR_ADDR                     0x68
#define MPU6886_WHO_AM_I_REG_ADDR               0x75
#define MPU6886_SMPLRT_DIV_REG_ADDR             0x19
#define MPU6886_CONFIG_REG_ADDR                 0x1A
#define MPU6886_ACCEL_CONFIG_REG_ADDR           0x1C
#define MPU6886_ACCEL_CONFIG_2_REG_ADDR         0x1D
#define MPU6886_FIFO_EN_REG_ADDR                0x23
#define MPU6886_INT_PIN_CFG_REG_ADDR            0x37
#define MPU6886_INT_ENABLE_REG_ADDR             0x38
#define MPU6886_ACCEL_XOUT_REG_ADDR             0x3B
#define MPU6886_USER_CRTL_REG_ADDR              0x6A
#define MPU6886_PWR_MGMT_1_REG_ADDR             0x6B
#define MPU6886_PWR_MGMT_2_REG_ADDR             0x6C

#define MPU6886_GYRO_XOUT_REG_ADDR 0x43

static const char *TAG = "SENSOR";

static float hp_alpha, lp_alpha;
static float prev_mag, prev_hp, prev_lp;

esp_err_t sensor_i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t mpu6886_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6886_SENSOR_ADDR, &reg_addr, 1, data, len, 10*I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t mpu6886_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6886_SENSOR_ADDR, write_buf, sizeof(write_buf), 10*I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

void init_mpu6886(void)
{
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, (0x01 << 7)));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, (0x01 << 0)));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_ACCEL_CONFIG_REG_ADDR, 0x18));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_CONFIG_REG_ADDR, 0x01));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_SMPLRT_DIV_REG_ADDR, 0x05));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_ENABLE_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_ACCEL_CONFIG_2_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_USER_CRTL_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_FIFO_EN_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_PIN_CFG_REG_ADDR, 0x22));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_ENABLE_REG_ADDR, 0x01));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "MPU6866 initialized successfully");
}

void getAccelAdc(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    mpu6886_register_read(MPU6886_ACCEL_XOUT_REG_ADDR, buf, 6);
    *ax = ((int16_t)buf[0] << 8) | buf[1];
    *ay = ((int16_t)buf[2] << 8) | buf[3];
    *az = ((int16_t)buf[4] << 8) | buf[5];
}

void getAccelData(float *ax, float *ay, float *az)
{
    int16_t accX = 0, accY = 0, accZ = 0;
    getAccelAdc(&accX, &accY, &accZ);
    float aRes = 16.0 / 32768.0;
    *ax = accX * aRes;
    *ay = accY * aRes;
    *az = accZ * aRes;
}

void getGyroAdc(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6];
    mpu6886_register_read(MPU6886_GYRO_XOUT_REG_ADDR, buf, 6);
    *gx = ((int16_t)buf[0] << 8) | buf[1];
    *gy = ((int16_t)buf[2] << 8) | buf[3];
    *gz = ((int16_t)buf[4] << 8) | buf[5];
}

void getGyroData(float *gx, float *gy, float *gz)
{
    int16_t rawX, rawY, rawZ;
    getGyroAdc(&rawX, &rawY, &rawZ);
    float gRes = 2000.0 / 32768.0;
    *gx = rawX * gRes;
    *gy = rawY * gRes;
    *gz = rawZ * gRes;
}

void get_mpu_readings(float *ax, float *ay, float *az, float *gx, float *gy, float *gz)
{
    getAccelData(ax, ay, az);
    getGyroData(gx, gy, gz);
}

esp_err_t init_accel_filters(float fs, float hp_fc, float lp_fc)
{
    float dt = 1.0f / fs;
    float rc_hp = 1.0f / (2.0f * M_PI * hp_fc);
    hp_alpha = rc_hp / (rc_hp + dt);
    float rc_lp = 1.0f / (2.0f * M_PI * lp_fc);
    lp_alpha = dt / (rc_lp + dt);
    prev_mag = prev_hp = prev_lp = 0.0f;
    return ESP_OK;
}

void filter_accel_mag(float ax, float ay, float az, float *filtered_mag)
{
    float mag = sqrtf(ax*ax + ay*ay + az*az);
    float hp = hp_alpha * (prev_hp + mag - prev_mag);
    float lp = prev_lp + lp_alpha * (hp - prev_lp);
    prev_mag = mag;
    prev_hp  = hp;
    prev_lp  = lp;
    *filtered_mag = lp;
}
