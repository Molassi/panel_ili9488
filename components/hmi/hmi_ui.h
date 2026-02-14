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

// Toma los valores editables por el usuario y los imprime en la pantalla.
void ui_config_draw_values(void);

// Imprime indicador en estado CONFIG.
void ui_draw_selection(void);

// Modifica el campo de seleccion.
void ui_config_next_field(void);

void cfg_apply_delta_selected(int);


#ifdef __cplusplus
}
#endif
