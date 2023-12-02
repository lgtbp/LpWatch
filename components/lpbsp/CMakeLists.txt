idf_component_register(SRCS "lpbsp.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_adc esp_event
    esp_wifi spi_flash
    nvs_flash esp_timer 
    fatfs 
    efuse
    )