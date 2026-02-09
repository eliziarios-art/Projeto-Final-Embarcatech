#ifndef SOIL_H
#define SOIL_H

#include <stdint.h>

uint16_t read_soil_adc(void);
float map_percent(uint16_t adc);
void soil_hw_init(void);

#endif
