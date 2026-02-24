#include "hmi.h"
#include "hmi_ui.h"
#include "hmi_events.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "app.h"
#include "Display_ili9488_35.h"

static const char *TAG = "HMI_UI";


// Siempre comienzo desde CFG_CANT_CORTES.
static cfg_field_t s_cfg_sel = CFG_CANT_CORTES;

// Posiciones pantalla, coordenadas donde va el indicador.
    const int x_val = 100;
    const int x_mark = 20;
    const int y_cant_cor = 75;
    const int y_offset1 = 100;
    const int y_offset2 = 125;

// Estados de la UI
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

void cfg_apply_delta_selected(int delta)
{
    app_config_t cfg = app_get_config();

    switch (s_cfg_sel) {
        case CFG_CANT_CORTES:
            app_set_cant_cortes(cfg.cant_cortes + delta);
            break;

        case CFG_OFF1:
            app_set_offset1(cfg.offset1 + delta);
            break;

        case CFG_OFF2:
            app_set_offset2(cfg.offset2 + delta);
            break;

        default:
            break;
    }
}

// Manejo de eventos según estado
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

// Imprime indicador en estado CONFIG.
void ui_draw_selection(void)
{
    // CANT CORTES
    if(s_cfg_sel == CFG_CANT_CORTES){
        display_ili9488_35_draw_text_8x8_rot90(y_cant_cor, x_mark, "-", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
    }else{
        display_ili9488_35_draw_text_8x8_rot90(y_cant_cor, x_mark, " ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
    }

    // OFFSET 1
    if(s_cfg_sel == CFG_OFF1){
        display_ili9488_35_draw_text_8x8_rot90(y_offset1, x_mark, "-", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
    }else{
        display_ili9488_35_draw_text_8x8_rot90(y_offset1, x_mark, " ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
    }

    // OFFSET 2
    if(s_cfg_sel == CFG_OFF2){
        display_ili9488_35_draw_text_8x8_rot90(y_offset2, x_mark, "-", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
    }else{
        display_ili9488_35_draw_text_8x8_rot90(y_offset2, x_mark, " ", 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
    }
}

// Toma los valores editables por el usuario y los imprime en la pantalla.
void ui_config_draw_values(void)
{
    char buf[32];
    app_config_t cfg = app_get_config();

    // CANT CORTES
    snprintf(buf, sizeof(buf), "%5d", cfg.cant_cortes);
    display_ili9488_35_draw_text_8x8_rot90(y_cant_cor, x_val, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

    // OFFSET 1
    snprintf(buf, sizeof(buf), "%5d", cfg.offset1);
    display_ili9488_35_draw_text_8x8_rot90(100, x_val, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);

    // OFFSET 2
    snprintf(buf, sizeof(buf), "%5d", cfg.offset2);
    display_ili9488_35_draw_text_8x8_rot90(125, x_val, buf, 0xFFFF, 0x0000, 2, DISP_ROT_90_CCW);
}

// Modifica el campo de seleccion.
void ui_config_next_field(void)
{
    s_cfg_sel = (cfg_field_t)((s_cfg_sel + 1) % CFG_COUNT);
}



// ES PARA QUITAR ui_config_draw_offset1(bool highlight) - MAS GENERAL Y MEJOR - Fase de prueba aún.
void ui_config_draw_field(bool editing)
{
    char buf[16];
    app_config_t cfg = app_get_config();

    int value = 0;
    int y = 0;

    // 1) Determinar qué valor y qué posición usar
    switch (s_cfg_sel)
    {
        case CFG_CANT_CORTES:
            value = cfg.cant_cortes;
            y = 75;
            break;

        case CFG_OFF1:
            value = cfg.offset1;
            y = 100;
            break;

        case CFG_OFF2:
            value = cfg.offset2;
            y = 125;
            break;

        default:
            return;
    }

    // 2) Determinar si se debe invertir
    //bool highlight = (editing && (field == s_cfg_sel));
    bool highlight = editing;

    uint16_t fg = highlight ? 0x0000 : 0xFFFF;
    uint16_t bg = highlight ? 0xFFFF : 0x0000;

    snprintf(buf, sizeof(buf), "%5d", value);

    display_ili9488_35_draw_text_8x8_rot90(y, 100, buf, fg, bg, 2, DISP_ROT_90_CCW);
}

 