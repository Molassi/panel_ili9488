#include "Display_ili9488_35.h"

#include <assert.h>
#include <stddef.h>
#include "esp_log.h"

#include "esp_heap_caps.h"
#include "esp_lcd_ili9488.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_dev.h"

/* Colores RGB565 */
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define WHITE 0xFFFF
#define BLACK 0x0000

static const char *TAG = "Display check:";

// --- Estado interno del componente ---
static esp_lcd_panel_io_handle_t s_io = NULL;
static esp_lcd_panel_handle_t    s_panel = NULL;

static int s_hres  = 320;
static int s_vres  = 480;
static int s_lines = 40;

// Buffer DMA para lo que manda la app (RGB565)
static uint16_t *s_buf565 = NULL;

esp_err_t display_ili9488_35_init(const display_ili9488_35_cfg_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;

    s_hres  = (cfg->hres  > 0) ? cfg->hres  : 320;
    s_vres  = (cfg->vres  > 0) ? cfg->vres  : 480;
    s_lines = (cfg->lines > 0) ? cfg->lines : 40;

    // 1) SPI bus
    spi_bus_config_t buscfg = {
        .sclk_io_num = cfg->pin_sclk,
        .mosi_io_num = cfg->pin_mosi,
        .miso_io_num = cfg->pin_miso,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = s_hres * s_lines * sizeof(uint16_t), // app envía RGB565
    };
    ESP_ERROR_CHECK(spi_bus_initialize(cfg->host, &buscfg, SPI_DMA_CH_AUTO));

    // 2) Panel IO (SPI)
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = cfg->pin_dc,
        .cs_gpio_num = cfg->pin_cs,
        .pclk_hz = cfg->pclk_hz,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)cfg->host, &io_config, &s_io));

    // 3) Panel ILI9488 (SPI requiere 18bpp)
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = cfg->pin_rst,
        .rgb_endian = cfg->rgb_endian,
        .bits_per_pixel = 18, // CLAVE: panel trabaja en RGB666
    };

    // CLAVE: buffer_size para conversión RGB565 -> RGB666 (3 bytes/pixel)
    const size_t buffer_size = (size_t)s_hres * (size_t)s_lines * 3;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(s_io, &panel_config, buffer_size, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    // 4) Buffer DMA RGB565 (lado app)
    s_buf565 = heap_caps_malloc((size_t)s_hres * (size_t)s_lines * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!s_buf565) return ESP_ERR_NO_MEM;

    return ESP_OK;
}

esp_err_t display_ili9488_35_fill_rgb565(uint16_t color)
{
    if (!s_panel || !s_buf565) {
        return ESP_ERR_INVALID_STATE;
    }

    const int pixels = s_hres * s_lines;
    for (int i = 0; i < pixels; i++) {
        s_buf565[i] = color;
    }

    for (int y = 0; y < s_vres; y += s_lines) {
        int y2 = y + s_lines;
        if (y2 > s_vres) y2 = s_vres;

        esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, y, s_hres, y2, s_buf565);
        if (err != ESP_OK) return err;
    }

    return ESP_OK;
}

void display_debug_cycle(void){
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