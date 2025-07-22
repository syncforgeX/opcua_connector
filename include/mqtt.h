#ifndef _MQTT_H_
#define _MQTT_H_

#include "device_config.h"
#include <MQTTClient.h>
#include <stdlib.h>
#include <signal.h>

/* QUEUE SETTINGS */
#define QUEUE_CAPACITY 5 // Max queue size

/* JSON DATA SIZE */
#define MAX_JSON_SIZE 1000


#define CLEAR                     0U
#define SET                       1U
#define INIT_VAL                  0U

/* ERROR CODE */
#define E_OK                    0
#define ENOT_OK                 1

/* MQTT TIMER CONFIGURATION */
#define MQTT_TIMER_INITIAL_START (getenv("MQTT_TIMER_INITIAL_START") ? atoi(getenv("MQTT_TIMER_INITIAL_START")) : 2)
#define MQTT_INTERVAL (getenv("MQTT_INTERVAL") ? atoi(getenv("MQTT_INTERVAL")) : 2) // Publish interval

/* RECONNECT SETTINGS */
#define RECONNECT_CNT (getenv("RECONNECT_CNT") ? atoi(getenv("RECONNECT_CNT")) : 4) // Max reconnection attempts

/* QUEUE SETTINGS */
//#define QUEUE_CAPACITY (getenv("QUEUE_CAPACITY") ? atoi(getenv("QUEUE_CAPACITY")) : 5) // Max queue size

/* FOCAS TIMER CONFIGURATION */
#define OPCUA_TIMER_INITIAL_START (getenv("FOCAS_TIMER_INITIAL_START") ? atoi(getenv("FOCAS_TIMER_INITIAL_START")) : 1)
//#define OPCUA_INTERVAL (getenv("FOCAS_INTERVAL") ? atoi(getenv("FOCAS_INTERVAL")) : 1) // Polling interval

#define FILE_PATH (getenv("FOCAS_FILE_PATH") ? getenv("FOCAS_FILE_PATH") : "buffer_data/focas_buffer.json")

/* MQTT CONFIGURATION */
//#define ADDRESS (getenv("MQTT_BROKER") ? getenv("MQTT_BROKER") : "tcp://localhost:1883")
//#define CLIENTID (getenv("MQTT_CLIENTID") ? getenv("MQTT_CLIENTID") : "LocalClient1234")
//#define TOPIC (getenv("MQTT_TOPIC") ? getenv("MQTT_TOPIC") : "test/topic")
//#define QOS (getenv("MQTT_QOS") ? atoi(getenv("MQTT_QOS")) : 1) // At least once delivery guarantee

#define TIMEOUT (getenv("MQTT_TIMEOUT") ? atol(getenv("MQTT_TIMEOUT")) : 10000L) // 10-second timeout
#define MQTT_CERTS_PATH (getenv("MQTT_CERTS_PATH") ? getenv("MQTT_CERTS_PATH") : "/etc/opcua_connector/certs/mqtt_cert.pem") //certs path

/* JSON DATA SIZE */
//#define MAX_JSON_SIZE (getenv("MAX_JSON_SIZE") ? atoi(getenv("MAX_JSON_SIZE")) : 1000)


#include "mqtt_queue.h"

typedef struct {
    char json_payload[MAX_JSON_SIZE];  // Holds the final JSON payload
    bool json_ready;          // Flag to indicate if the payload is ready
} mqtt_data_st;

uint8_t data_collection(OPCUAValue *);
uint8_t mqtt_init(void);
bool mqtt_deinit(void);
static bool mqtt_client_deinit(void);
static bool mqtt_client_init();
void write_payloads_to_file(Queue *queue);
void write_payloads_to_csv(Queue *queue);
bool readfile_publish_init(void);
void publish_message(const char *message);

#endif
