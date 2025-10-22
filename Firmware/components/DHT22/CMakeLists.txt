idf_component_register(SRCS "DHT22.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver hw_gpio hw_timer data_processing)
