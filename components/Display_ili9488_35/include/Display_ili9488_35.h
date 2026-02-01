#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_vendor.h"

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

#ifdef __cplusplus
}
#endif
