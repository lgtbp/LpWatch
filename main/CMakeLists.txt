file(GLOB CORE "*.c")
file(GLOB RESOURCE "./resource/*.c")
idf_component_register(SRCS
                        ${CORE} 
                        ${RESOURCE}
                        INCLUDE_DIRS "."
                        )