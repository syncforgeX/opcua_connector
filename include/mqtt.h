#ifndef _MQTT_H_
#define _MQTT_H_

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
#define MQTT_INTERVAL		2
#include "device_config.h"

extern volatile uint8_t DataDelay_cntr;

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
