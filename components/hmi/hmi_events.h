#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // Botones (short press)
    HMI_EVT_BTN1_SHORT = 0,
    HMI_EVT_BTN2_SHORT,
    HMI_EVT_BTN3_SHORT,

    // Futuro (ejemplos):
    HMI_EVT_DATA_DIRTY,     // “cambió algo que se muestra”
    HMI_EVT_GOTO_MAIN,      // comando interno
    HMI_EVT_GOTO_CONFIG,    // comando interno
    HMI_EVT_START_SEQUENCE, // comando interno
} hmi_event_t;

#ifdef __cplusplus
}
#endif
