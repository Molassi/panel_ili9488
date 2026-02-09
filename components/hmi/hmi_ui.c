#include "hmi.h"
#include "hmi_ui.h"
#include "hmi_events.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "app.h"
#include "Display_ili9488_35.h"

// ======================================================
// LOG
// ======================================================
static const char *TAG = "HMI_UI";

// ======================================================
// Estados de la UI
// ======================================================
typedef enum {
    UI_STATE_SPLASH = 0,
    UI_STATE_MAIN,
    UI_STATE_WORK,
    UI_STATE_CONFIG
} ui_state_t;

// ======================================================
// Contexto interno de la UI
// ======================================================
typedef struct {
    ui_state_t state;
} ui_ctx_t;

static ui_ctx_t ui_ctx;

// ======================================================
// Prototipos internos
// ======================================================
static void ui_draw_full(ui_state_t state);
static void ui_handle_event(hmi_event_t evt);

// ======================================================
// API interna llamada desde hmi.c
// ======================================================
void hmi_ui_task(void *arg)
{
    QueueHandle_t q = (QueueHandle_t)arg;

    // -------------------------------
    // Estado inicial: SPLASH
    // -------------------------------
    ui_ctx.state = UI_STATE_SPLASH;
    ui_draw_full(UI_STATE_SPLASH);

    // El delay del splash se maneja en hmi.c
    // (acá solo reaccionamos a eventos)

    ESP_LOGI(TAG, "UI task started, waiting events");

    while (1) {
        hmi_event_t evt;

        // 100% event-driven
        xQueueReceive(q, &evt, portMAX_DELAY);

        // Evento de actualización de datos (sin cambio de estado)
        if (evt == HMI_EVT_DATA_DIRTY) {
            ESP_LOGD(TAG, "DATA_DIRTY");
            // Acá luego:
            // ui_update_dynamic();
            continue;
        }

        ui_handle_event(evt);
    }
}

// ======================================================
// Manejo de eventos según estado
// ======================================================
static void ui_handle_event(hmi_event_t evt)
{
    switch (ui_ctx.state) {

        case UI_STATE_MAIN:
            if (evt == HMI_EVT_BTN3_SHORT) {
                ui_ctx.state = UI_STATE_WORK;
                ui_draw_full(ui_ctx.state);
            }
            else if (evt == HMI_EVT_BTN2_SHORT) {
                ui_ctx.state = UI_STATE_CONFIG;
                ui_draw_full(ui_ctx.state);
            }
            break;

        case UI_STATE_CONFIG:
            // Ejemplo: BTN2 vuelve a MAIN
            if (evt == HMI_EVT_BTN2_SHORT) {
                ui_ctx.state = UI_STATE_MAIN;
                ui_draw_full(ui_ctx.state);
            }
            break;

        case UI_STATE_WORK:
            // Ejemplo: BTN1 vuelve a MAIN
            if (evt == HMI_EVT_BTN1_SHORT) {
                ui_ctx.state = UI_STATE_MAIN;
                ui_draw_full(ui_ctx.state);
            }
            break;

        case UI_STATE_SPLASH:
            // En splash ignoramos botones
            break;

        default:
            break;
    }
}

// ======================================================
// Dibujo completo de pantalla (placeholder)
// ======================================================
static void ui_draw_full(ui_state_t state)
{
    switch (state) {

        case UI_STATE_SPLASH:
            ESP_LOGI(TAG, "[DRAW] SPLASH screen");
            // lcd_clear();
            // lcd_draw_logo();
            break;

        case UI_STATE_MAIN:
            ESP_LOGI(TAG, "[DRAW] MAIN screen");
            // lcd_clear();
            // lcd_draw_main_static();
            break;

        case UI_STATE_WORK:
            ESP_LOGI(TAG, "[DRAW] WORK screen");
            // lcd_clear();
            // lcd_draw_work_static();
            break;

        case UI_STATE_CONFIG:
            ESP_LOGI(TAG, "[DRAW] CONFIG screen");
            // lcd_clear();
            // lcd_draw_config_static();
            break;

        default:
            break;
    }
}


// Toma los valores editables por el usuario y los imprime en la pantalla.
void ui_config_draw_values(void)
{
    char buf[32];
    app_config_t cfg = app_get_config();

    // Elegí coordenadas donde van los números (ajustalas a tu layout)
    // OJO: vos estás usando rot90 y coordenadas (y,x). Mantené ese criterio.

    // CANT CORTES
    snprintf(buf, sizeof(buf), "%d", cfg.cant_cortes);
    display_ili9488_35_draw_text_8x8_rot90(75, 100, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

    // OFFSET 1
    snprintf(buf, sizeof(buf), "%d", cfg.offset1);
    display_ili9488_35_draw_text_8x8_rot90(100, 100, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

    // OFFSET 2
    snprintf(buf, sizeof(buf), "%d", cfg.offset2);
    display_ili9488_35_draw_text_8x8_rot90(125, 100, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
}
