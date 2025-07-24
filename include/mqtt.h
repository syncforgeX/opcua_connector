#ifndef _MQTT_H_
#define _MQTT_H_

#include "device_config.h"
#include <MQTTClient.h>
#include <stdlib.h>
#include <signal.h>

/* QUEUE SETTINGS */
#define QUEUE_CAPACITY 20 // Max queue size

/* JSON DATA SIZE */
#define MAX_JSON_SIZE 1000


#define CLEAR                     0U
#define SET                       1U
#define INIT_VAL                  0U

/* ERROR CODE */
#define E_OK                    0
#define ENOT_OK                 1

/* MQTT TIMER CONFIGURATION */
#define MQTT_TIMER_INITIAL_START 2
#define MQTT_INTERVAL 2 // Publish interval

/* RECONNECT SETTINGS */
#define RECONNECT_CNT 15 // Max reconnection attempts

/* FOCAS TIMER CONFIGURATION */
#define OPCUA_TIMER_INITIAL_START 1
#define OPCUA_INTERVAL 1 // Polling interval

#define OPCUA_FILE_PATH "buffer_data/focas_buffer.json"

/* MQTT CONFIGURATION */
#define TIMEOUT 10000L // 10-second timeout
#define MQTT_CERTS_PATH "/etc/opcua_connector/certs/mqtt_cert.pem" //certs path

//#define QUEUE_CAPACITY (getenv("QUEUE_CAPACITY") ? atoi(getenv("QUEUE_CAPACITY")) : 5) // Max queue size
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
