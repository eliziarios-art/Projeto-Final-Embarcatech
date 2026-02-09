#include "system_state.h"
#include <string.h>

system_state_t g_system_state;
SemaphoreHandle_t stateMutex;

/* =====================
   INIT
   ===================== */
void system_state_init(void) {

    stateMutex = xSemaphoreCreateMutex();

    memset(&g_system_state, 0, sizeof(system_state_t));

    /* Valores padrão */
    g_system_state.soil_min_limit = 30.0f;
    g_system_state.soil_max_limit = 45.0f;
    g_system_state.mode = MODE_AUTO;
}

/* =====================
   SETTERS
   ===================== */

void system_state_set_soil(float percent, sensor_status_t status) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);

    g_system_state.soil_percent = percent;
    g_system_state.soil_status = status;

    xSemaphoreGive(stateMutex);
}

void system_state_set_aht10(float temp, float hum, sensor_status_t status) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);

    g_system_state.temperature = temp;
    g_system_state.humidity = hum;
    g_system_state.aht10_status = status;

    xSemaphoreGive(stateMutex);
}

void system_state_set_limits(float min, float max) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);

    g_system_state.soil_min_limit = min;
    g_system_state.soil_max_limit = max;

    xSemaphoreGive(stateMutex);
}

void system_state_set_mode(system_mode_t mode)
{
    xSemaphoreTake(stateMutex, portMAX_DELAY);

    g_system_state.mode = mode;

    if(mode == MODE_EMERGENCY)
        g_system_state.pump_on = false;

    if(mode == MODE_AUTO)
        g_system_state.pump_on = false; // evita herdar estado manual

    xSemaphoreGive(stateMutex);
}

void system_state_set_pump(bool on)
{
    xSemaphoreTake(stateMutex, portMAX_DELAY);

    /* Emergência SEMPRE força OFF */
    if(g_system_state.mode == MODE_EMERGENCY)
    {
        g_system_state.pump_on = false;
    }
    else
    {
        g_system_state.pump_on = on;
    }

    xSemaphoreGive(stateMutex);
}
