#include "app.h"
#include "hmi.h"          // para hmi_post_event()
#include "hmi_events.h"   // eventos HMI

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "APP";

// ======================================================
// Configuración
// ======================================================
#define APP_TASK_STACK   4096
#define APP_TASK_PRIO    8
#define APP_TASK_CORE    1

#define APP_UPDATE_MS    500   // no actualizar HMI más rápido que esto

typedef struct {
    int value;             // variable de ejemplo 
} app_ctx_t;

static app_ctx_t app_ctx;

static app_config_t s_user_cfg = {
    .cant_cortes = 1000,
    .offset1 = 100,
    .offset2 = 500,
};

// Controlo que el valor este dentro de los limites.
static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Devuelve estructura con valores de parametros
app_config_t app_get_config(void)
{
    return s_user_cfg;
}

// modificación de variables.
void app_set_cant_cortes(int value)
{
    // Límites para cantidad de cortes
    const int OFFSET_MIN = 0;
    const int OFFSET_MAX = 1000;
    
    int newv = clamp_int(value, OFFSET_MIN, OFFSET_MAX);
    if (newv == s_user_cfg.cant_cortes){
        return;
    }

    s_user_cfg.cant_cortes = newv;
    ESP_LOGI(TAG, "cant_cortes=%d", s_user_cfg.cant_cortes);

    // avisar a HMI que hay datos nuevos.
    (void)hmi_post_event(HMI_EVT_DATA_DIRTY);
}
void app_set_offset1(int value)
{
    // Límites para offset1
    const int OFFSET_MIN = 0;
    const int OFFSET_MAX = 200;
    
    int newv = clamp_int(value, OFFSET_MIN, OFFSET_MAX);
    if (newv == s_user_cfg.offset1){
        return;
    }

    s_user_cfg.offset1 = newv;
    ESP_LOGI(TAG, "offset1=%d", s_user_cfg.offset1);

    // avisar a HMI que hay datos nuevos.
    (void)hmi_post_event(HMI_EVT_DATA_DIRTY);
}
void app_set_offset2(int value)
{
    // Límites para offset2
    const int OFFSET_MIN = 0;
    const int OFFSET_MAX = 1000;
    
    int newv = clamp_int(value, OFFSET_MIN, OFFSET_MAX);
    if (newv == s_user_cfg.offset2){
        return;
    }

    s_user_cfg.offset2 = newv;
    ESP_LOGI(TAG, "offset1=%d", s_user_cfg.offset2);

    // avisar a HMI que hay datos nuevos.
    (void)hmi_post_event(HMI_EVT_DATA_DIRTY);
}
// ======================================================
// Tarea principal del sistema
// ======================================================
static void app_task(void *arg)
{
    TickType_t last_wake = xTaskGetTickCount();

    ESP_LOGI(TAG, "App task started");

    while (1) {

        // ------------------------------------------------
        // Acá va tu lógica principal de proceso
        // ------------------------------------------------
            //app_ctx.value++;

            //ESP_LOGI(TAG, "Proceso ejecutándose, value=%d", app_ctx.value);

        // ------------------------------------------------
        // Notificar a la HMI que hay datos nuevos
        // (throttleado a 500 ms)
        // ------------------------------------------------
        // NOTA: PARA HACER PRUEBAS LO QUITO UN TOQUE.
            //hmi_post_event(HMI_EVT_DATA_DIRTY);

        // ------------------------------------------------
        // Periodicidad controlada
        // ------------------------------------------------
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_UPDATE_MS));
    }
}

// ======================================================
// API pública
// ======================================================
void app_init(void)
{
    app_ctx.value = 0;
    xTaskCreatePinnedToCore(app_task,"app_task",APP_TASK_STACK,NULL,APP_TASK_PRIO,NULL,APP_TASK_CORE);
    ESP_LOGI(TAG, "app_init OK");
}
