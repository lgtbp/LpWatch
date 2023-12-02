

idf_component_register(SRCS 
    INCLUDE_DIRS "."
    )

add_prebuilt_library(target_lp_verify "${CMAKE_CURRENT_LIST_DIR}/liblpverify.a" PRIV_REQUIRES lpbsp 
                    app_update miniz_300 mbedtls cJSON
                    )
target_link_libraries(${COMPONENT_LIB} INTERFACE target_lp_verify)
