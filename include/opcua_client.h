#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include <pthread.h>
#include <stdint.h>
// Thread function to monitor OPC UA data points
static void *opcua_client_thread(void *arg);

uint8_t opcua_init();
void opcua_deinit();
#endif // OPCUA_CLIENT_H

