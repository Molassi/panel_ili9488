#include "hmi.h"
#include "hmi_ui.h"
#include "hmi_events.h"
#include "hmi_buttons.h"
#include "app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "COLVEN_LOGO.c"
#include "Display_ili9488_35.h"

static const char *TAG = "HMI";

// =================== Config (ajustá a tu HW) ===================
#define HMI_SPLASH_MS   1500

// GPIOs definidos como pulsadores de HMI.
#define HMI_BTN1_GPIO   GPIO_NUM_14
#define HMI_BTN2_GPIO   GPIO_NUM_33
#define HMI_BTN3_GPIO   GPIO_NUM_32

// Botones: polling/debounce
#define HMI_BTN_POLL_MS     10
#define HMI_BTN_DEBOUNCE_MS 40

// Texto para pantallas.
static const char *SK_MAIN   = "[ --- ]     [ CNF ]    [ INI ]";
static const char *SK_WORK   = "[ --- ]     [ --- ]    [ STP ]";
static const char *SK_CONFIG = "[ REG ]     [ OK  ]    [ BAJ ]";

// =================== Estado UI ===================
typedef enum {
    UI_SPLASH = 0,
    UI_MAIN   = 1,
    UI_WORK   = 2,
    UI_CONFIG = 3
} ui_state_t;

// =================== Globals internos ===================
static QueueHandle_t s_hmi_queue = NULL;
static TaskHandle_t  s_ui_task_handle = NULL;
static bool          s_buttons_started = false;

// =================== Draw (placeholder) ===================
static void draw_screen(ui_state_t st)
{
    // Maquina de estados de pantallas.
    switch (st) {
        // State 0: Pantalla inicio.
        case UI_SPLASH: ESP_LOGI(TAG, "[DRAW] SPLASH"); 
            display_ili9488_35_draw_rgb565_rot90(0, 0, 480, 320, COLVEN_LOGO_480_320, DISP_ROT_90_CCW);
        break;
        
        // State 1: Pantalla principal - 
        case UI_MAIN:   ESP_LOGI(TAG, "[DRAW] MAIN"); 
            display_ili9488_35_fill_rgb565(0x0000); // negro
            display_ili9488_35_draw_text_8x8_rot90(0, 0,   "           PRINCIPAL          ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
            display_ili9488_35_draw_text_8x8_rot90(300, 0, SK_MAIN, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW); // 0,0 (y,x) es la equina superior-derecha.
        break;
        
        // State 2: Trabajando
        case UI_WORK:   ESP_LOGI(TAG, "[DRAW] WORK"); 
             display_ili9488_35_fill_rgb565(0x0000); // negro
             display_ili9488_35_draw_text_8x8_rot90(0, 0,   "          TRABAJANDO          ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
             //display_ili9488_35_fill_rect_rgb565(300, 50, 6 * 8 * 2, 8 * 2, 0x0000);
             display_ili9488_35_draw_text_8x8_rot90(300, 0, SK_WORK, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
        break;

        // State 3: Seleccion de parametro a configurar.
        case UI_CONFIG: ESP_LOGI(TAG, "[DRAW] CONFIG");
            display_ili9488_35_fill_rgb565(0x0000); // negro
            display_ili9488_35_draw_text_8x8_rot90(0, 0,   "         CONFIGURACION        ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
            display_ili9488_35_draw_text_8x8_rot90(75, 0,  "CANT CORTES:                  ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
            display_ili9488_35_draw_text_8x8_rot90(100, 0, "OFFSET AVANCE 1:              ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
            display_ili9488_35_draw_text_8x8_rot90(125, 0, "OFFSET AVANCE 2:              ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
            display_ili9488_35_draw_text_8x8_rot90(300, 0, SK_CONFIG, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

            //Imprimo los valores
            ui_config_draw_values();
        break;
        
        default: break;
    }
}

// =================== UI Task ===================
static void ui_task(void *arg)
{
    ui_state_t state = UI_SPLASH;

    // 1) Splash - imagen de inicio.
    draw_screen(UI_SPLASH);
    vTaskDelay(pdMS_TO_TICKS(HMI_SPLASH_MS));

    // 2) Pasar a MAIN
    state = UI_MAIN;
    draw_screen(UI_MAIN);

    // 3) Recién ahora arrancar tarea de botones
    if (!s_buttons_started) {
        buttons_config_t cfg = {
            .btn1_gpio = HMI_BTN1_GPIO,
            .btn2_gpio = HMI_BTN2_GPIO,
            .btn3_gpio = HMI_BTN3_GPIO,
            .active_low = true,
            .poll_ms = HMI_BTN_POLL_MS,
            .debounce_ms = HMI_BTN_DEBOUNCE_MS,
            .task_priority = 6,
            .task_stack = 3072,
            .task_core = 0,
        };
        buttons_start(&cfg, s_hmi_queue);
        s_buttons_started = true;
        ESP_LOGI(TAG, "Buttons started after splash");
    }

    ESP_LOGI(TAG, "UI ready. Waiting events...");

    // 4) Event-driven: bloquea hasta que llegue un evento
    while (1) {
        hmi_event_t evt;
        xQueueReceive(s_hmi_queue, &evt, portMAX_DELAY);

        // Evento "DATA_DIRTY" (futuro): actualizar solo segmentos, sin redibujar todo.
        if (evt == HMI_EVT_DATA_DIRTY) {
            // update_dynamic_segments();
            ESP_LOGI(TAG, "[UI] DATA_DIRTY");
            continue;
        }

        switch (state) {
            case UI_MAIN:
                if (evt == HMI_EVT_BTN3_SHORT) {
                    state = UI_WORK;
                    draw_screen(state);
                } else if (evt == HMI_EVT_BTN2_SHORT) {
                    state = UI_CONFIG;
                    draw_screen(state);
                }
                break;

            case UI_CONFIG:
                // ejemplo: BTN2 vuelve a MAIN
                if (evt == HMI_EVT_BTN1_SHORT) {
                    state = UI_MAIN;
                    draw_screen(state);
                }
                break;

            case UI_WORK:
                // ejemplo: BTN1 vuelve a MAIN
                if (evt == HMI_EVT_BTN3_SHORT) {
                    state = UI_MAIN;
                    draw_screen(state);
                }
                break;

            default:
                break;
        }
    }
}

// =================== API pública ===================
void hmi_init(void)
{
    // Inicializo pantalla.
    static const display_ili9488_35_cfg_t cfg = {
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
    ESP_ERROR_CHECK(display_ili9488_35_init(&cfg));
    ESP_LOGI(TAG, "Init display component...");

    // Creo una queue de 16 filas, devuelve null si no se pudo crear y da falla.
    s_hmi_queue = xQueueCreate(16, sizeof(hmi_event_t));
    if (!s_hmi_queue) {
        ESP_LOGE(TAG, "Failed creating HMI queue");
        return;
    }

    // Crear UI task (secundaria)
    xTaskCreatePinnedToCore(ui_task, "hmi_ui_task", 4096, NULL, 5, &s_ui_task_handle, 1);

    ESP_LOGI(TAG, "hmi_init OK (ui_task on core1)");
}

bool hmi_post_event(hmi_event_t evt)
{
    if (!s_hmi_queue) return false;
    return (xQueueSend(s_hmi_queue, &evt, 0) == pdTRUE);
}
