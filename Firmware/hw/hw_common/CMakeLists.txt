idf_component_register(SRCS "hw_common.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver hw_gpio hw_i2c hw_adc)
