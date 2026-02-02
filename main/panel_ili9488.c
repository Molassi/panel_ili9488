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

    //display_debug_cycle(); //Se utiliza solo para probar hardware.

    // Inicio pantalla.
    display_ili9488_35_draw_rgb565_rot90(0, 0, 480, 320, COLVEN_LOGO_480_320, DISP_ROT_90_CCW);
    vTaskDelay(pdMS_TO_TICKS(3000));
    display_ili9488_35_fill_rgb565(0x0000); // negro

    // Primer prueba de escritura.
    display_ili9488_35_draw_text_8x8(10, 10, "COLVEN", 0xFFFF, 0x0000, 2);
    display_ili9488_35_draw_text_8x8(10, 50, "ABCDEFGHIJKLMNOPQRSTUVWXYZ []", 0xFFFF, 0x0000, 1);
    //display_ili9488_35_draw_text_8x8_rot90(200, 0, "TEST 123", 0xFFFF, 0x0000, 1, DISP_ROT_90_CCW);
    display_ili9488_35_draw_text_8x8_rot90(50, 100, "[HOLA]", 0xFFFF, 0x0000, 4, DISP_ROT_90_CCW);
    display_ili9488_35_draw_text_8x8_rot90(100, 200, "[HOLA1 PRECIOSA]", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

    //display_ili9488_35_draw_rgb565_rot90(10, 100, "Hola crack", 0xFFFF, 0x0000, s_rotation);

    int counter = 0;

    while (1) {
        // borrar área
        display_ili9488_35_fill_rect_rgb565(10, 50, 6 * 8 * 2, 8 * 2, 0x0000);

        // escribir nuevo valor
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", counter++);
        display_ili9488_35_draw_text_8x8_rot90(10, 50, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}
