#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_heap_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    spi_host_device_t host;

    int pin_sclk;
    int pin_mosi;
    int pin_miso;
    int pin_cs;
    int pin_dc;
    int pin_rst;

    int hres;
    int vres;
    int pclk_hz;
    int lines;

    lcd_color_rgb_endian_t rgb_endian;
} display_ili9488_35_cfg_t;

esp_err_t display_ili9488_35_init(const display_ili9488_35_cfg_t *cfg);
esp_err_t display_ili9488_35_fill_rgb565(uint16_t color);

void display_debug_cycle(void);

// Dibuja una imagen RGB565 (array uint16_t) en la región indicada.
// img_rgb565 debe tener w*h píxeles en orden fila-major (izq->der, arriba->abajo).
esp_err_t display_ili9488_35_draw_rgb565(int x, int y, int w, int h, const uint16_t *img_rgb565);

// Dibuja una imagen RGB565 de pantalla completa (s_hres x s_vres).
esp_err_t display_ili9488_35_draw_fullscreen_rgb565(const uint16_t *img_rgb565);

// Dibuja una imagen RGB565 rotada 90° (clockwise o counterclockwise).
// Útil si tu asset está en 480x320 y tu panel está en 320x480.
// La imagen fuente es (src_w x src_h). El destino debe entrar en la pantalla.
typedef enum {
    DISP_ROT_90_CW = 0,
    DISP_ROT_90_CCW = 1,
} display_rot90_t;

esp_err_t display_ili9488_35_draw_rgb565_rot90(int x, int y, int src_w, int src_h, const uint16_t *src_rgb565, display_rot90_t rot);


#ifdef __cplusplus
}
#endif
