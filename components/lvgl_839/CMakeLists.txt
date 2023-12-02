

idf_component_register(SRCS 
    INCLUDE_DIRS "."
    )
add_prebuilt_library(target_lvgl "${CMAKE_CURRENT_LIST_DIR}/liblvgl_839.a" PRIV_REQUIRES lpbsp)
target_link_libraries(${COMPONENT_LIB} INTERFACE target_lvgl)

