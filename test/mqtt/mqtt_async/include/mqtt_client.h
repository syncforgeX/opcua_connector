#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <MQTTAsync.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// Structure for MessageBus config
typedef struct {
    char *type;
    bool disabled;
    char *protocol;
    char *host;
    uint16_t port;
    char *authmode;
    char *secretname;
    char *clientid;
    int qos;
    int keepalive;
    bool retained;
    char *certfile;
    char *keyfile;
    bool skipverify;
    char *basetopicprefix;

    // Runtime fields
    MQTTAsync client;
    char *uri;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
	    bool connected;  // <-- 🔥 Add this line

} MessageBusConfig;

// Structure for MQTT config (wrapper)
typedef struct {
    MessageBusConfig messagebus;
    // Add other MQTT fields if needed
} MQTTConfig;

// Top-level Device config
typedef struct {
    char *device_name;
    MQTTConfig mqtt;
    // Add OPC UA, data points, etc., as needed
} DeviceConfig;

// Bus structure
typedef struct edgex_bus_t {
    void *ctx;
    bool (*postfn)(struct edgex_bus_t *bus, const char *topic, const char *msg);
    void (*freefn)(struct edgex_bus_t *bus);
    bool (*subsfn)(struct edgex_bus_t *bus, const char *topic);
} edgex_bus_t;

// Init & cleanup
edgex_bus_t *mqtt_client_init(DeviceConfig *config, const char *svcname);
void edgex_bus_mqtt_free(edgex_bus_t *bus);

// Publishing and subscribing
bool edgex_bus_mqtt_post(edgex_bus_t *bus, const char *topic, const char *msg);
bool edgex_bus_mqtt_subscribe(edgex_bus_t *bus, const char *topic);

// Optional helper
void edgex_bus_init(edgex_bus_t *bus, const char *svcname, DeviceConfig *cfg);

#endif // MQTT_CLIENT_H

