#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "Display_ili9488_35.h"

static const char *TAG = "APP";

/* Colores RGB565 */
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define WHITE 0xFFFF
#define BLACK 0x0000

void app_main(void)
{
    display_ili9488_35_cfg_t cfg = {
        .host = SPI3_HOST,
        .pin_sclk = 18,
        .pin_mosi = 23,
        .pin_miso = -1,
        .pin_cs   = 5,
        .pin_dc   = 2,
        .pin_rst  = -1,
        .hres     = 320,
        .vres     = 480,
        .pclk_hz  = 10 * 1000 * 1000,
        .lines    = 40,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
    };

    ESP_LOGI(TAG, "Init display component...");
    ESP_ERROR_CHECK(display_ili9488_35_init(&cfg));

    while (1) {
        ESP_LOGI(TAG, "ROJO");
        ESP_ERROR_CHECK(display_ili9488_35_fill_rgb565(RED));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "VERDE");
        ESP_ERROR_CHECK(display_ili9488_35_fill_rgb565(GREEN));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "AZUL");
        ESP_ERROR_CHECK(display_ili9488_35_fill_rgb565(BLUE));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "BLANCO");
        ESP_ERROR_CHECK(display_ili9488_35_fill_rgb565(WHITE));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "NEGRO");
        ESP_ERROR_CHECK(display_ili9488_35_fill_rgb565(BLACK));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
