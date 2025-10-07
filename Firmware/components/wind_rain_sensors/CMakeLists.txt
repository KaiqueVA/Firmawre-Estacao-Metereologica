idf_component_register(SRCS "wind_rain_sensors.c"
                    INCLUDE_DIRS "include"
                    REQUIRES hw_gpio driver hw_timer hw_adc)
