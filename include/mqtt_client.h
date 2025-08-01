#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <MQTTAsync.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "device_config.h"

int mqtt_init();
int mqtt_deinit();
typedef bool (*edgex_bus_postfn)(void *ctx, const char *topic, const char *msg);
typedef bool (*edgex_bus_subsfn)(void *ctx, const char *topic);
typedef void (*edgex_bus_freefn)(void *ctx);

// Message bus structure
typedef struct edgex_bus_t {
    void *ctx;                            // Context (typically MessageBusConfig*)
    edgex_bus_postfn postfn;             // Publish function
    edgex_bus_subsfn subsfn;             // Subscribe function
    edgex_bus_freefn freefn;             // Free function
    char *prefix;                        // Topic prefix
    char *svcname;                       // Service name
    pthread_mutex_t mtx;                 // Mutex for thread safety
    bool msgb64payload;                  // Base64 encode payload?
} edgex_bus_t;

// Init & cleanup
edgex_bus_t *mqtt_client_init(DeviceConfig *config, const char *svcname);
//void edgex_bus_mqtt_free(edgex_bus_t *bus);

// Optional helper
void edgex_bus_init(edgex_bus_t *bus, const char *svcname, DeviceConfig *cfg);

#endif // MQTT_CLIENT_H

