//main.c
// Display and sensor congigurations
#define CONFIG_WIDTH 135
#define CONFIG_HEIGHT 240
#define CONFIG_MOSI_GPIO 15
#define CONFIG_SCLK_GPIO 13
#define CONFIG_CS_GPIO 5 
#define CONFIG_DC_GPIO 23
#define CONFIG_RESET_GPIO 18
#define CONFIG_BL_GPIO -1
#define CONFIG_LED_GPIO 10
#define CONFIG_OFFSETX 52
#define CONFIG_OFFSETY 40

// Mode toggle: 1 = stream via TCP, 0 = run step counter
#define CONNECT_TO_SERVER 0

// ESP-IDF core libraries
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

// Project-specific includes
#include "sensor.h"                   
#include "protocol_examples_common.h" 
#include "axp192.h"
#include "st7789.h"
#include "fontx.h"
#include "esp_spiffs.h"

// Logging tag for SPIFFS
static const char *TAG = "SPIFFS";

// Font structure for large fonts
static FontxFile fxBig[2];

// External functions
extern void step_counter_task(void *pvParameters);
extern esp_err_t sensor_i2c_master_init(void);
extern void init_mpu6886(void);
extern void tcp_client(void *pvParameters);

// Initializes SPIFFS filesystem for storing fonts
void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
    size_t total, used;
    ESP_ERROR_CHECK(esp_spiffs_info(NULL, &total, &used));
    ESP_LOGI(TAG, "SPIFFS mounted (total: %u, used: %u)", total, used);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
     if (CONNECT_TO_SERVER) {
        ESP_ERROR_CHECK(example_connect());
    }

    // Initialize I2C and MPU sensor
    ESP_ERROR_CHECK(sensor_i2c_master_init());
    init_mpu6886();
    ESP_ERROR_CHECK(init_accel_filters(100.0f, 0.6f, 3.0f));

    // Display
    AXP192_PowerOn();
    AXP192_ScreenBreath(11);
    static TFT_t dev;
    spi_master_init(&dev,
        CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO,
        CONFIG_CS_GPIO,   CONFIG_DC_GPIO,
        CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev,
        CONFIG_WIDTH,     CONFIG_HEIGHT,
        CONFIG_OFFSETX,   CONFIG_OFFSETY,
        true);
    
    // Fonts
    init_spiffs();
    InitFontx(fxBig, "/spiffs/ILGH32XB.FNT", "/spiffs/ILMH32XB.FNT");
    OpenFontx(&fxBig[0]);
    OpenFontx(&fxBig[1]);

    typedef struct {
        TFT_t     *dev;
        FontxFile *fx;
    } step_args_t;

    static step_args_t args;
    args.dev = &dev;   
    args.fx  = fxBig;


    xTaskCreate(step_counter_task, "step_counter", 6 * 1024, &args, 5, NULL);

    if (CONNECT_TO_SERVER) {
        xTaskCreate(tcp_client, "tcp_client", 8 * 1024, &dev, 5, NULL);
    }

}
