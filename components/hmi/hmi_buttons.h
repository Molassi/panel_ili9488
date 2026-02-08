#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t btn1_gpio;
    gpio_num_t btn2_gpio;
    gpio_num_t btn3_gpio;

    bool active_low;      // true: pressed=0 (pull-up). false: pressed=1 (pull-down)
    int poll_ms;          // recomendado 10..20ms
    int debounce_ms;      // recomendado 30..60ms

    int task_priority;    // ej: 6
    int task_stack;       // ej: 3072
    int task_core;        // 0 o 1 (o -1 para no fijar)
} buttons_config_t;

/**
 * @brief Inicializa GPIO de botones y arranca la tarea de polling/debounce.
 *        Cuando detecta "pressed" estable, envía un hmi_event_t a out_queue.
 */
void buttons_start(const buttons_config_t *cfg, QueueHandle_t out_queue);

#ifdef __cplusplus
}
#endif
