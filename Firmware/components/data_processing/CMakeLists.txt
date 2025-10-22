idf_component_register(SRCS "data_processing.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver hw_timer bmp280 DHT22)
