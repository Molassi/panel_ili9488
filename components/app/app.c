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

// ======================================================
// Contexto de la aplicación
// ======================================================
typedef struct {
    int value;             // variable de ejemplo (luego será lo que quieras)
} app_ctx_t;

static app_ctx_t app_ctx;

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

    xTaskCreatePinnedToCore(
        app_task,
        "app_task",
        APP_TASK_STACK,
        NULL,
        APP_TASK_PRIO,
        NULL,
        APP_TASK_CORE
    );

    ESP_LOGI(TAG, "app_init OK");
}
