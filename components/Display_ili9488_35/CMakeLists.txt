idf_component_register(
    SRCS "Display_ili9488_35.c"                       
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_lcd_ili9488 freertos driver)