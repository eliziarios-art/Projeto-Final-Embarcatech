#ifndef AHT10_H
#define AHT10_H

#include <stdbool.h>

void aht10_hw_init(void);
void aht10_init(void);
bool aht10_read(float *temperature, float *humidity);

#endif
