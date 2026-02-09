#include "wifi_manager.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

bool wifi_connect(const char *ssid, const char *password)
{
    printf("Inicializando driver WiFi...\n");

    if (cyw43_arch_init()) {
        printf("Erro ao iniciar driver WiFi\n");
        return false;
    }

    printf("Driver WiFi OK\n");

    cyw43_arch_enable_sta_mode();

    printf("Modo STA OK\n");

    printf("Event group criado\n");

    printf("Conectando em %s...\n", ssid);

    if (cyw43_arch_wifi_connect_timeout_ms(
            ssid,
            password,
            CYW43_AUTH_WPA2_AES_PSK,
            30000)) {

        printf("Falha WiFi\n");
        return false;
    }

    printf("WiFi OK\n");

    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

    return true;
}
