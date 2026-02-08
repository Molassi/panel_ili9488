idf_component_register(
    SRCS
        "hmi.c"
        "hmi_ui.c"
        "hmi_buttons.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES freertos driver esp_lcd Display_ili9488_35
)

