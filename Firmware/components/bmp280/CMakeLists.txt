idf_component_register(SRCS "bmp280.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver hw_i2c data_processing)
