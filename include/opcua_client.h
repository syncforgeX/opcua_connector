#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include <pthread.h>
#include <stdint.h>
#include <signal.h>
// Thread function to monitor OPC UA data points
static void *opcua_client_thread(void *arg);
#define MAX_JSON_SIZE 2000
typedef struct {
    char json_payload[MAX_JSON_SIZE];  // Holds the final JSON payload
    bool json_ready;          // Flag to indicate if the payload is ready
} mqtt_data_st;

uint8_t opcua_init();
void opcua_deinit();
#endif // OPCUA_CLIENT_H

