// LOGICA PRINCIPAL (Proceso)

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el módulo de aplicación.
 *
 * Crea la tarea principal del sistema (app_task).
 * Esta es la lógica de proceso, NO la HMI.
 */
void app_init(void);

#ifdef __cplusplus
}
#endif
