idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver wind_rain_sensors hw_common data_processing bmp280 DHT22 LDR)
