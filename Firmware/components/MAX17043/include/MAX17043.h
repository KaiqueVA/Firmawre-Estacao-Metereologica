#ifndef MAX17043_H
#define MAX17043_H

typedef struct{
    float voltage;
    float soc;
} MAX17043_Data;

void init_MAX17043(void);

#endif // MAX17043_H