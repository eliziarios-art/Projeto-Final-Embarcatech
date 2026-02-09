#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "FreeRTOS.h"
#include "event_groups.h"

#define WIFI_CONNECTED_BIT (1 << 0)

extern EventGroupHandle_t wifi_event_group;

#include <stdbool.h>

bool wifi_connect(const char *ssid, const char *password);

#endif
