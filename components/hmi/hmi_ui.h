#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tarea de la interfaz HMI (UI).
 *
 * Esta función es interna del componente HMI.
 * - Implementada en hmi_ui.c
 * - Creada desde hmi.c
 * - Recibe como argumento la cola de eventos (QueueHandle_t)
 */
void hmi_ui_task(void *arg);

#ifdef __cplusplus
}
#endif
