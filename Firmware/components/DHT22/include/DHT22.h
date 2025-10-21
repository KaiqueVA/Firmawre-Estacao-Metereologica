#ifndef DHT22_H_  
#define DHT22_H_

#include "driver/gpio.h"

typedef struct{
    uint16_t temperature;
    uint16_t humidity;
} DHT22_data_t;

void dht_init(void);
int dht_read(DHT22_data_t *data);

#endif // DHT22_H_
