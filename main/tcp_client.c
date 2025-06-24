#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "esp_netif.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sensor.h"
#include "st7789.h"

#if defined(CONFIG_EXAMPLE_IPV4)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#endif
#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "TCP_CLIENT";

void tcp_client(void *pvParameters)
{
    (void)pvParameters;
    
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    inet_pton(AF_INET, HOST_IP_ADDR, &dest.sin_addr);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed: %d", errno);
        vTaskDelete(NULL);
        return;
    }
    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) != 0) {
        ESP_LOGE(TAG, "connect failed: %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Connected to %s:%d", HOST_IP_ADDR, PORT);

    BiquadState bp = {0};
    
    // Prime the filter
    float ax, ay, az, gx, gy, gz;
    get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
    float mag = sqrtf(ax*ax + ay*ay + az*az);
    biquad_bandpass(mag, &bp);

    int64_t start = esp_timer_get_time();
    while (esp_timer_get_time() - start < 20 * 1000000) {
        get_mpu_readings(&ax, &ay, &az, &gx, &gy, &gz);
        mag = sqrtf(ax*ax + ay*ay + az*az);
        float filtered = biquad_bandpass(mag, &bp);
        int64_t now = esp_timer_get_time();

        char line[128];
        snprintf(line, sizeof(line), "%lld,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                 now, (double)mag, (double)filtered, 
                 (double)ax, (double)ay, (double)az);
        send(sock, line, strlen(line), 0);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    send(sock, "end\n", 4, 0);
    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}