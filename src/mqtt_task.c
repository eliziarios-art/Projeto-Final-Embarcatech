#include "mqtt_task.h"
#include "system_state.h"
#include "wifi_manager.h"

#include "lwip/apps/mqtt.h"
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

#include <stdio.h>
#include <string.h>

#define BROKER_PORT 1883
#define MQTT_BROKER_IP "192.168.15.90"

static mqtt_client_t *client;
static bool mqtt_connected = false;

/* Guarda o tópico atual */
static char mqtt_topic[64];


// ======================
// CALLBACK PUBLISH
// ======================

static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if(result != ERR_OK)
        printf("MQTT: Erro publish: %d\n", result);
}


// ======================
// CALLBACK RECEBER TÓPICO
// ======================

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    strncpy(mqtt_topic, topic, sizeof(mqtt_topic));
    mqtt_topic[sizeof(mqtt_topic)-1] = '\0';

    printf("MQTT: Mensagem no topico: %s\n", mqtt_topic);
}


// ======================
// CALLBACK RECEBER DADOS
// ======================

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    char msg[32] = {0};

    if(len >= sizeof(msg))
        len = sizeof(msg) - 1;

    memcpy(msg, data, len);

    printf("MQTT RX [%s]: %s\n", mqtt_topic, msg);

    /* ======================
       CONTROLE DE MODO
       ====================== */

    if(strcmp(mqtt_topic, "irrigador/cmd/mode") == 0)
    {
        if(strcmp(msg, "AUTO") == 0)
            system_state_set_mode(MODE_AUTO);

        else if(strcmp(msg, "MANUAL") == 0)
            system_state_set_mode(MODE_MANUAL);

        else if(strcmp(msg, "EMERGENCY") == 0)
            system_state_set_mode(MODE_EMERGENCY);
    }

    /* ======================
       CONTROLE BOMBA
       ====================== */

    if(strcmp(mqtt_topic, "irrigador/cmd/pump") == 0)
    {
        if(strcmp(msg, "ON") == 0)
            system_state_set_pump(true);

        else if(strcmp(msg, "OFF") == 0)
            system_state_set_pump(false);
    }
}


// ======================
// CALLBACK CONEXÃO MQTT
// ======================

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if(status == MQTT_CONNECT_ACCEPTED)
    {
        printf("MQTT: *** CONECTADO COM SUCESSO! ***\n");

        mqtt_connected = true;

        mqtt_set_inpub_callback(
            client,
            mqtt_incoming_publish_cb,
            mqtt_incoming_data_cb,
            NULL
        );

        /* Assina tópicos */

        mqtt_subscribe(client, "irrigador/cmd/pump", 0, NULL, NULL);
        mqtt_subscribe(client, "irrigador/cmd/mode", 0, NULL, NULL);
    }
    else
    {
        printf("MQTT: Falha conexao status=%d\n", status);
        mqtt_connected = false;
    }
}


// ======================
// TASK MQTT
// ======================

void task_mqtt(void *p)
{
    printf("MQTT: Aguardando WiFi...\n");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        false,
        true,
        portMAX_DELAY
    );

    struct netif *net = netif_default;

    if(net)
        printf("MQTT: Pico W Online. IP: %s\n",
               ip4addr_ntoa(netif_ip4_addr(net)));

    ip_addr_t broker_ip;
    ipaddr_aton(MQTT_BROKER_IP, &broker_ip);

    client = mqtt_client_new();

    struct mqtt_connect_client_info_t ci;
    memset(&ci, 0, sizeof(ci));

    ci.client_id = "pico_irrigador";
    ci.keep_alive = 60;

    err_t err = mqtt_client_connect(
        client,
        &broker_ip,
        BROKER_PORT,
        mqtt_connection_cb,
        NULL,
        &ci
    );

    if(err != ERR_OK)
        printf("MQTT: Erro iniciar conexao: %d\n", err);


    /* LOOP TELEMETRIA */

    while (1)
    {
        if(mqtt_connected && mqtt_client_is_connected(client))
        {
            system_state_t snapshot;

            if(xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                snapshot = g_system_state;
                xSemaphoreGive(stateMutex);

                char payload[128];

                snprintf(payload, sizeof(payload),
                    "{\"soil\":%.1f,\"temp\":%.1f,\"humidity\":%.1f,\"pump\":%d,\"mode\":%d}",
                    snapshot.soil_percent,
                    snapshot.temperature,
                    snapshot.humidity,
                    snapshot.pump_on,
                    snapshot.mode
                );

                mqtt_publish(
                    client,
                    "irrigador/telemetry",
                    payload,
                    strlen(payload),
                    0,
                    0,
                    mqtt_pub_request_cb,
                    NULL
                );
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
