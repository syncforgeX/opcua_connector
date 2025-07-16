#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include <pthread.h>

// Thread function to monitor OPC UA data points
void *opcua_client_thread(void *arg);

#endif // OPCUA_CLIENT_H

