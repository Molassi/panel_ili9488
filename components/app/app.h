// LOGICA PRINCIPAL (Proceso)

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Estructura de parametros modificables por el usuario.
typedef struct {
    int cant_cortes;
    int offset1;   
    int offset2;
} app_config_t;

// Devuelve estructura con valores de parametros
app_config_t app_get_config(void);

// Cantidad de cortes
void app_set_cant_cortes(int value);
// Seteo offset1
void app_set_offset1(int value);
// Seteo offset2
void app_set_offset2(int value);

void app_init(void);

#ifdef __cplusplus
}
#endif
