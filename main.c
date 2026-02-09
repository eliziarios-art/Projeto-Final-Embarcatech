#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "soil.h"
#include "aht10.h"
#include "system_state.h"
#include "tasks.h"
#include "wifi_manager.h"
#include "pico/cyw43_arch.h"
EventGroupHandle_t wifi_event_group;



int main() {
    stdio_init_all();
    sleep_ms(5000);

    printf("\n=== FreeRTOS START ===\n");

    system_state_init();

    wifi_event_group = xEventGroupCreate();
  
    soil_hw_init();
    aht10_hw_init();
    aht10_init();

    


    create_tasks();

    vTaskStartScheduler();

    while (1) {}
}
