#include "hmi.h"
#include "hmi_ui.h"
#include "hmi_events.h"
#include "hmi_buttons.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

static const char *TAG = "HMI";

// =================== Config (ajustá a tu HW) ===================
#define HMI_SPLASH_MS   1500

// Pines estándar de tu producto (cambiá por los tuyos)
#define HMI_BTN1_GPIO   GPIO_NUM_18
#define HMI_BTN2_GPIO   GPIO_NUM_19
#define HMI_BTN3_GPIO   GPIO_NUM_21

// Botones: polling/debounce
#define HMI_BTN_POLL_MS     10
#define HMI_BTN_DEBOUNCE_MS 40

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
    // Acá luego conectás tu LCD_ILI9488 (dibujar pantallas)
    switch (st) {
        case UI_SPLASH: ESP_LOGI(TAG, "[DRAW] SPLASH"); break;
        case UI_MAIN:   ESP_LOGI(TAG, "[DRAW] MAIN"); break;
        case UI_WORK:   ESP_LOGI(TAG, "[DRAW] WORK"); break;
        case UI_CONFIG: ESP_LOGI(TAG, "[DRAW] CONFIG"); break;
        default: break;
    }
}

// =================== UI Task ===================
static void ui_task(void *arg)
{
    ui_state_t state = UI_SPLASH;

    // 1) Splash una sola vez
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
                if (evt == HMI_EVT_BTN2_SHORT) {
                    state = UI_MAIN;
                    draw_screen(state);
                }
                break;

            case UI_WORK:
                // ejemplo: BTN1 vuelve a MAIN
                if (evt == HMI_EVT_BTN1_SHORT) {
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
