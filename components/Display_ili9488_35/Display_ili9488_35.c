#include "Display_ili9488_35.h"

#include <assert.h>
#include <stddef.h>
#include "esp_log.h"
#include <string.h>
#include "esp_heap_caps.h"

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

esp_err_t display_ili9488_35_draw_rgb565(int x, int y, int w, int h, const uint16_t *img_rgb565)
{
    if (!s_panel || !img_rgb565) return ESP_ERR_INVALID_STATE;
    if (w <= 0 || h <= 0) return ESP_ERR_INVALID_ARG;

    // límites pantalla
    if (x < 0 || y < 0 || (x + w) > s_hres || (y + h) > s_vres) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, (void *)img_rgb565);
}

esp_err_t display_ili9488_35_draw_fullscreen_rgb565(const uint16_t *img_rgb565)
{
    return display_ili9488_35_draw_rgb565(0, 0, s_hres, s_vres, img_rgb565);
}

esp_err_t display_ili9488_35_draw_rgb565_rot90(int x, int y, int src_w, int src_h, const uint16_t *src_rgb565, display_rot90_t rot)
{
    if (!s_panel || !src_rgb565) return ESP_ERR_INVALID_STATE;
    if (src_w <= 0 || src_h <= 0) return ESP_ERR_INVALID_ARG;

    // Dimensiones destino luego de rotar 90°
    const int dst_w = src_h;
    const int dst_h = src_w;

    // límites pantalla
    if (x < 0 || y < 0 || (x + dst_w) > s_hres || (y + dst_h) > s_vres) {
        return ESP_ERR_INVALID_ARG;
    }

    // Usamos un bloque de líneas destino (por ejemplo s_lines)
    const int lines = s_lines;
    const int block_h = (lines > 0) ? lines : 40;

    // Buffer DMA para un bloque destino: dst_w * block_h
    uint16_t *blk = heap_caps_malloc((size_t)dst_w * (size_t)block_h * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!blk) return ESP_ERR_NO_MEM;

    for (int dst_y = 0; dst_y < dst_h; dst_y += block_h) {
        int cur_h = block_h;
        if (dst_y + cur_h > dst_h) cur_h = dst_h - dst_y;

        // Rellenar bloque rotado: para cada pixel destino (dx, dy) calculamos fuente (sx, sy)
        for (int dy = 0; dy < cur_h; dy++) {
            const int y_out = dst_y + dy;

            for (int dx = 0; dx < dst_w; dx++) {
                int sx, sy;

                if (rot == DISP_ROT_90_CW) {
                    // (dst_x, dst_y) = rot90CW(src_x, src_y)
                    // dst_w = src_h, dst_h = src_w
                    // sx = y_out
                    // sy = src_h - 1 - dx
                    sx = y_out;
                    sy = src_h - 1 - dx;
                } else {
                    // rot90CCW
                    // sx = src_w - 1 - y_out
                    // sy = dx
                    sx = src_w - 1 - y_out;
                    sy = dx;
                }

                blk[(size_t)dy * (size_t)dst_w + (size_t)dx] = src_rgb565[(size_t)sy * (size_t)src_w + (size_t)sx];
            }
        }

        // Dibujar este bloque en pantalla
        esp_err_t err = esp_lcd_panel_draw_bitmap(
            s_panel,
            x, y + dst_y,
            x + dst_w, y + dst_y + cur_h,
            blk
        );
        if (err != ESP_OK) {
            heap_caps_free(blk);
            return err;
        }
    }

    heap_caps_free(blk);
    return ESP_OK;
}

static const uint8_t font8x8_min[ ((']' - ' ') + 1) ][8] = {
    // ' ' (32)
    [ ' ' - ' ' ] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},

    // '-' (45)
    [ '-' - ' ' ] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},

    // ':' (58)
    [ ':' - ' ' ] = {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},

    // '0'..'9' (48-57)
    [ '0' - ' ' ] = {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    [ '1' - ' ' ] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    [ '2' - ' ' ] = {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00},
    [ '3' - ' ' ] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    [ '4' - ' ' ] = {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
    [ '5' - ' ' ] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    [ '6' - ' ' ] = {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    [ '7' - ' ' ] = {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00},
    [ '8' - ' ' ] = {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    [ '9' - ' ' ] = {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},

    // 'A'..'Z' (65-90)
    [ 'A' - ' ' ] = {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    [ 'B' - ' ' ] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    [ 'C' - ' ' ] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    [ 'D' - ' ' ] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    [ 'E' - ' ' ] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    [ 'F' - ' ' ] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    [ 'G' - ' ' ] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
    [ 'H' - ' ' ] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    [ 'I' - ' ' ] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    [ 'J' - ' ' ] = {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},
    [ 'K' - ' ' ] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    [ 'L' - ' ' ] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    [ 'M' - ' ' ] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    [ 'N' - ' ' ] = {0x66,0x76,0x7E,0x6E,0x66,0x66,0x66,0x00},
    [ 'O' - ' ' ] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    [ 'P' - ' ' ] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    [ 'Q' - ' ' ] = {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00},
    [ 'R' - ' ' ] = {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
    [ 'S' - ' ' ] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    [ 'T' - ' ' ] = {0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00},
    [ 'U' - ' ' ] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    [ 'V' - ' ' ] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    [ 'W' - ' ' ] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    [ 'X' - ' ' ] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    [ 'Y' - ' ' ] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    [ 'Z' - ' ' ] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},

    // Símbolos adicionales (91-93)
    [ '[' - ' ' ] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    [ ']' - ' ' ] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
};

static inline const uint8_t *glyph8x8(char c)
{
    if (c < ' ' || c > 'Z') c = ' ';
    const uint8_t *g = font8x8_min[c - ' '];
    // si no está definido (todo 0), lo dejamos como espacio
    return g;
}

// Dibuja un char 8x8 escalado (scale=1..N)
static esp_err_t draw_char8x8_scaled(int x, int y, char c, uint16_t fg, uint16_t bg, int scale)
{
    if (scale < 1) scale = 1;

    const uint8_t *g = glyph8x8(c);

    // buffer temporal: 8*scale por 8*scale
    const int w = 8 * scale;
    const int h = 8 * scale;

    // OJO: para tamaños grandes conviene hacerlo por líneas, pero para prueba está ok.
    uint16_t pix[w * h];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            int on = (bits >> (7 - col)) & 1;  // MSB->LSB
            uint16_t color = on ? fg : bg;

            // pintar bloque scale x scale
            for (int yy = 0; yy < scale; yy++) {
                for (int xx = 0; xx < scale; xx++) {
                    int dx = col * scale + xx;
                    int dy = row * scale + yy;
                    pix[dy * w + dx] = color;
                }
            }
        }
    }

    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, pix);
}

esp_err_t display_ili9488_35_draw_text_8x8(int x, int y, const char *txt, uint16_t fg, uint16_t bg, int scale)
{
    if (!s_panel || !txt) return ESP_ERR_INVALID_STATE;

    int cx = x;
    int cy = y;
    const int step = 8 * (scale < 1 ? 1 : scale);

    for (size_t i = 0; txt[i]; i++) {
        char c = txt[i];
        if (c == '\n') {
            cy += step + (scale * 2);
            cx = x;
            continue;
        }

        esp_err_t err = draw_char8x8_scaled(cx, cy, c, fg, bg, scale);
        if (err != ESP_OK) return err;
        cx += step;
    }

    return ESP_OK;
}


// Reusa tu función interna actual que dibuja UN char o tu tabla font.
// Asumo que ya tenés un mecanismo de font dentro del .c.
// Si tu función de texto actual ya pinta chars usando un glyph 8x8,
// acá reutilizamos el mismo glyph y dibujamos sobre un buffer.

static inline const uint8_t *glyph8x8(char c); // <- si ya la tenés, BORRAR esta línea
// Si NO tenés glyph8x8, decime y te adapto al método que estés usando.

esp_err_t display_ili9488_35_draw_text_8x8_rot90(int x, int y, const char *txt,
                                                 uint16_t fg, uint16_t bg, int scale,
                                                 display_rot90_t rot)
{
    if (!txt) return ESP_ERR_INVALID_ARG;
    if (scale < 1) scale = 1;

    // 1) calcular tamaño del bloque de texto (simple: una sola línea)
    // Si querés soportar '\n', lo ampliamos luego.
    const int len = (int)strlen(txt);
    const int char_w = 8 * scale;
    const int char_h = 8 * scale;

    const int w = len * char_w;
    const int h = char_h;

    // 2) reservar buffer RGB565 en DMA (mejor, así draw_bitmap no sufre)
    uint16_t *buf = heap_caps_malloc((size_t)w * (size_t)h * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buf) return ESP_ERR_NO_MEM;

    // 3) fondo
    for (int i = 0; i < w * h; i++) buf[i] = bg;

    // 4) renderizar cada char en el buffer (8x8 escalado)
    for (int ci = 0; ci < len; ci++) {
        char c = txt[ci];
        if (c < 32 || c > 127) c = ' ';

        const uint8_t *g = glyph8x8(c);  // debe devolver 8 bytes (filas)

        const int x0 = ci * char_w;

        for (int row = 0; row < 8; row++) {
            uint8_t bits = g[row];

            for (int col = 0; col < 8; col++) {
                int on = (bits >> (7 - col)) & 1;  // MSB->LSB
                if (!on) continue;

                // pintar bloque escalado
                for (int yy = 0; yy < scale; yy++) {
                    for (int xx = 0; xx < scale; xx++) {
                        int px = x0 + col * scale + xx;
                        int py = row * scale + yy;
                        buf[py * w + px] = fg;
                    }
                }
            }
        }
    }

    // 5) dibujar el buffer rotado usando TU función existente (no tocamos el resto)
    esp_err_t err = display_ili9488_35_draw_rgb565_rot90(x, y, w, h, buf, rot);

    heap_caps_free(buf);
    return err;
}


