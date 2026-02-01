#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "COLVEN_LOGO.c"

#include "Display_ili9488_35.h"         //componente

static const char *TAG = "APP_main:";

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

    display_ili9488_35_draw_rgb565_rot90(0, 0, 480, 320, COLVEN_LOGO_480_320, DISP_ROT_90_CCW);


    //display_debug_cycle(); //Se utiliza solo para probar hardware.

    while (1)
    {
        vTaskDelay(1);
    }
    
}
