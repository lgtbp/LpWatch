idf_component_register(SRCS "lplib.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl_839 lpbsp miniz_300 minmea_030 
    mbedtls cJSON
    )