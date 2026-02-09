#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR
} sensor_status_t;

typedef enum {
    MODE_AUTO = 0,
    MODE_MANUAL,
    MODE_EMERGENCY
} system_mode_t;


typedef struct {
    // Solo
    uint16_t soil_adc;
    float soil_percent;
    sensor_status_t soil_status;

    // Ambiente
    float temperature;
    float humidity;
    sensor_status_t aht10_status;

    // Atuador
    bool pump_on;
    
    // Controle
    system_mode_t mode;
    float soil_min_limit;
    float soil_max_limit;
    
} system_state_t;


void system_state_set_soil(float percent, sensor_status_t status);
void system_state_set_aht10(float temp, float hum, sensor_status_t status);

void system_state_set_limits(float min, float max);
void system_state_set_mode(system_mode_t mode);
void system_state_set_pump(bool on);

extern system_state_t g_system_state;
extern SemaphoreHandle_t stateMutex;

void system_state_init(void);

#endif
