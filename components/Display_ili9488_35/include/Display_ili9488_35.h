#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------
// Configuración del display
// ---------------------------
typedef struct {
    spi_host_device_t host;

    int pin_sclk;
    int pin_mosi;
    int pin_miso;   // -1 si no usás
    int pin_cs;
    int pin_dc;
    int pin_rst;    // -1 si reset directo

    int hres;       // ej: 320
    int vres;       // ej: 480
    int pclk_hz;    // ej: 10*1000*1000
    int lines;      // ej: 40

    lcd_color_rgb_endian_t rgb_endian;
} display_ili9488_35_cfg_t;

// ---------------------------
// Rotación 90° para imágenes
// (lo que ya venís usando)
// ---------------------------
typedef enum {
    DISP_ROT_90_CW = 0,
    DISP_ROT_90_CCW = 1,
} display_rot90_t;

// ---------------------------
// API privada del componente
// ---------------------------
void display_debug_cycle(void);

// ---------------------------
// API pública del componente
// ---------------------------
esp_err_t display_ili9488_35_init(const display_ili9488_35_cfg_t *cfg);

// Llenar pantalla con un color RGB565 (usa el buffer interno por líneas)
esp_err_t display_ili9488_35_fill_rgb565(uint16_t color);

// Dibujar una imagen RGB565 (sin rotación) en región x,y,w,h
esp_err_t display_ili9488_35_draw_rgb565(int x, int y, int w, int h, const uint16_t *img_rgb565);

// Dibujar una imagen RGB565 rotada 90° CW/CCW (útil para assets 480x320)
esp_err_t display_ili9488_35_draw_rgb565_rot90(int x, int y, int src_w, int src_h, const uint16_t *src_rgb565, display_rot90_t rot);

// Texto simple 8x8 escalable
esp_err_t display_ili9488_35_draw_text_8x8(int x, int y, const char *txt, uint16_t fg, uint16_t bg, int scale);

// Texto simple 8x8 escalable rotado 90°
esp_err_t display_ili9488_35_draw_text_8x8_rot90(int x, int y, const char *txt, uint16_t fg, uint16_t bg, int scale, display_rot90_t rot);

// Rectangulo para ocultar textos.
esp_err_t display_ili9488_35_fill_rect_rgb565(int x, int y, int w, int h, uint16_t color);

#ifdef __cplusplus
}
#endif

