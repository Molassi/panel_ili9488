/*
hmi.c         - Contiene: init(), cola y creación de tareas.
hmi_ui.c      - Contiene: maquina de estados de hmi y dibuja las pantallas 
hmi.buttons.c - Contiene: pollings de pulsadores.
hmi.events.c  - Contiene: eventos HMI internos.
*/

#pragma once

#include "hmi_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa HMI:
 *  - crea cola de eventos
 *  - crea ui_task
 *  - muestra splash y luego habilita buttons_task
 */
void hmi_init(void);

/**
 * @brief (Opcional) Permite que otros módulos posteen eventos a la HMI
 *        (ej: HMI_EVT_DATA_DIRTY). Retorna false si no pudo.
 */
bool hmi_post_event(hmi_event_t evt);

#ifdef __cplusplus
}
#endif
