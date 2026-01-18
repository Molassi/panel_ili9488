#include <stdio.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"

#include "driver/spi_master.h"

#include "esp_lcd_ili9488.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_interface.h"
//#include "esp_lcd_panel_io_spi.h"
#include "esp_lcd_panel_dev.h"

static const char *TAG = "ILI9488_TEST";

/* -------- Pinout -------- */
#define LCD_HOST          SPI3_HOST
#define PIN_NUM_LCD_SCLK  18
#define PIN_NUM_LCD_MOSI  23
#define PIN_NUM_LCD_MISO  -1
#define PIN_NUM_LCD_CS     5
#define PIN_NUM_LCD_DC     2
#define PIN_NUM_LCD_RST   -1   // reset directo

/* -------- Display -------- */
#define LCD_H_RES          320
#define LCD_V_RES          480
#define LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)  // luego subimos

/* -------- Colores RGB565 -------- */
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define WHITE 0xFFFF
#define BLACK 0x0000

/* -------- Handles -------- */
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

/* -------- Buffer DMA RGB565 -------- */
#define LINES 40
static uint16_t *g_buf565 = NULL;

static void display_init(void)
{
    ESP_LOGI(TAG, "Init SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_LCD_SCLK,
        .mosi_io_num = PIN_NUM_LCD_MOSI,
        .miso_io_num = PIN_NUM_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LINES * sizeof(uint16_t), // lo que mandamos desde app (RGB565)
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Create panel IO (SPI)");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle)
    );

    ESP_LOGI(TAG, "Create ILI9488 panel (panel en 18bpp, app envia RGB565)");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR, // si rojo/azul invertidos, cambiar a RGB
        .bits_per_pixel = 18,             // CLAVE: el panel trabaja en RGB666
    };

    // CLAVE: buffer_size para el driver: 3 bytes/pixel (RGB666), aunque tu app mande RGB565
    size_t buffer_size = LCD_H_RES * LINES * 3;
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_ili9488(io_handle, &panel_config, buffer_size, &panel_handle)
    );

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Alloc DMA buffer RGB565");
    g_buf565 = heap_caps_malloc(LCD_H_RES * LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(g_buf565);
}

static void fill_screen_color_565(uint16_t color)
{
    const int PIXELS = LCD_H_RES * LINES;
    for (int i = 0; i < PIXELS; i++) {
        g_buf565[i] = color;
    }
    for (int y = 0; y < LCD_V_RES; y += LINES) {
        ESP_ERROR_CHECK(
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_H_RES, y + LINES, g_buf565)
        );
    }
}

void app_main(void)
{
    display_init();

    while (1) {
        ESP_LOGI(TAG, "ROJO");
        fill_screen_color_565(RED);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "VERDE");
        fill_screen_color_565(GREEN);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "AZUL");
        fill_screen_color_565(BLUE);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "BLANCO");
        fill_screen_color_565(WHITE);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "NEGRO");
        fill_screen_color_565(BLACK);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
