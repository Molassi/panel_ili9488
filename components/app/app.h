// LOGICA PRINCIPAL (Proceso)

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Variables modificable por el usuario.
typedef struct {
    int cant_cortes;
    int offset1;   
    int offset2;
} app_config_t;

app_config_t app_get_config(void);

void app_init(void);

#ifdef __cplusplus
}
#endif
