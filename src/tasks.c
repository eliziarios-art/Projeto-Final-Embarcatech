#include "system_state.h"
#include "tasks.h"

#include "soil.h"
#include "aht10.h"
#include "mqtt_task.h"
#include "wifi_manager.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pico/stdlib.h"
#include <stdio.h>

#define WIFI_SSID "VICTOR"
#define WIFI_PASS "130719vale"


/* =====================
   DEFINIÇÕES LOCAIS
   ===================== */
#define LED_PIN 11

typedef struct {
    uint16_t adc;
    float soil_percent;
} soil_data_t;

typedef struct {
    float temperature;
    float humidity;
} aht10_data_t;

QueueHandle_t soilQueue;
QueueHandle_t aht10Queue;

/* =====================
   TASK SOLO
   ===================== */
void task_soil(void *p) {
    soil_data_t data;

    while (1) {
        data.adc = read_soil_adc();
        data.soil_percent = map_percent(data.adc);

        if (data.adc == 0 || data.adc >= 4095) {
            data.soil_percent = 0;
        }

        xQueueOverwrite(soilQueue, &data);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* =====================
   TASK AHT10
   ===================== */
void task_aht10(void *p) {
    aht10_data_t data;

    while (1) {
        if (aht10_read(&data.temperature, &data.humidity)) {
            if (data.temperature > -40 && data.temperature < 85 &&
                data.humidity >= 0 && data.humidity <= 100) {

                xQueueOverwrite(aht10Queue, &data);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* =====================
   TASK STATE MANAGER
   ===================== */
void task_state_manager(void *p) {
    soil_data_t soil;
    aht10_data_t air;

    while (1) {

        if (xQueueReceive(soilQueue, &soil, 0) == pdPASS) {
            xSemaphoreTake(stateMutex, portMAX_DELAY);

            g_system_state.soil_adc = soil.adc;
            g_system_state.soil_percent = soil.soil_percent;
            g_system_state.soil_status =
                (soil.adc == 0 || soil.adc >= 4095) ? SENSOR_ERROR : SENSOR_OK;

            xSemaphoreGive(stateMutex);
        }

        if (xQueueReceive(aht10Queue, &air, 0) == pdPASS) {
            xSemaphoreTake(stateMutex, portMAX_DELAY);

            g_system_state.temperature = air.temperature;
            g_system_state.humidity = air.humidity;
            g_system_state.aht10_status = SENSOR_OK;

            xSemaphoreGive(stateMutex);
        }


        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* =====================
   TASK CONTROLLER
   ===================== */
void task_controller(void *p) {

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {

        float soil_percent;
        sensor_status_t soil_status;
        system_mode_t mode;
        float min_limit;
        float max_limit;
        bool pump_on;

        xSemaphoreTake(stateMutex, portMAX_DELAY);

        soil_percent = g_system_state.soil_percent;
        soil_status = g_system_state.soil_status;
        mode = g_system_state.mode;
        min_limit = g_system_state.soil_min_limit;
        max_limit = g_system_state.soil_max_limit;
        pump_on = g_system_state.pump_on;

        xSemaphoreGive(stateMutex);

        /* ===== EMERGÊNCIA ===== */
        if (mode == MODE_EMERGENCY) {

            gpio_put(LED_PIN, 0);
            system_state_set_pump(false);

            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        /* ===== MANUAL ===== */
        if (mode == MODE_MANUAL) {

            xSemaphoreTake(stateMutex, portMAX_DELAY);
            bool pump = g_system_state.pump_on;
            xSemaphoreGive(stateMutex);

            gpio_put(LED_PIN, pump);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        /* ===== AUTO ===== */
        if (soil_status == SENSOR_ERROR) {

            gpio_put(LED_PIN, 0);
            system_state_set_pump(false);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!pump_on && soil_percent < min_limit) {

            pump_on = true;
            gpio_put(LED_PIN, 1);
            system_state_set_pump(true);

        }

        else if (pump_on && soil_percent > max_limit) {

            pump_on = false;
            gpio_put(LED_PIN, 0);
            system_state_set_pump(false);

        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

}



/* =====================
   TASK LOGGER
   ===================== */
void task_logger(void *p) {
    system_state_t snapshot;

    printf("LOGGER STARTED\n");

    while (1) {
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        snapshot = g_system_state;
        xSemaphoreGive(stateMutex);

        printf("Solo: %.1f %% (%s) | ",
            snapshot.soil_percent,
            snapshot.soil_status == SENSOR_OK ? "OK" : "ERRO");

        printf("Temp: %.1f C | Umid: %.1f %% (%s) | ",
            snapshot.temperature,
            snapshot.humidity,
            snapshot.aht10_status == SENSOR_OK ? "OK" : "ERRO");

        printf("Bomba: %s\n",
               snapshot.pump_on ? "ON" : "OFF");

        printf("Modo: %d | ", snapshot.mode);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void task_wifi(void *p)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    wifi_connect(WIFI_SSID, WIFI_PASS);

    vTaskDelete(NULL);
}

/* =====================
   CREATE TASKS
   ===================== */
void create_tasks(void) {
    soilQueue = xQueueCreate(1, sizeof(soil_data_t));
    aht10Queue = xQueueCreate(1, sizeof(aht10_data_t));

    xTaskCreate(task_wifi, "WiFi", 4096, NULL, 4, NULL);
    xTaskCreate(task_mqtt, "MQTT", 2048, NULL, 2, NULL);
    xTaskCreate(task_soil, "Soil", 512, NULL, 1, NULL);
    xTaskCreate(task_aht10, "AHT10", 1024, NULL, 1, NULL);
    xTaskCreate(task_state_manager, "StateMgr", 768, NULL, 2, NULL);
    xTaskCreate(task_controller, "Control", 512, NULL, 2, NULL);
    xTaskCreate(task_logger, "Logger", 768, NULL, 1, NULL);


}
