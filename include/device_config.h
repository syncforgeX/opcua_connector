#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#define MAX_DATA_POINTS 5

#include <MQTTAsync.h>
#include <open62541/client.h>
#include <stdlib.h>

#define CLEAR                     0U
#define SET                       1U
#define INIT_VAL                  0U
/* ERROR CODE */
#define E_OK                    0
#define ENOT_OK                 1

#define OPCUA_TIMER_INITIAL_START 1
#define MQTT_INTERVAL 2

typedef struct {
    int namespace;
    int identifier;
    char datatype[32];
    char alias[64];
    char nodeid[128];  // Optional: human-readable format like "ns=3;i=1001"
} DataPoint;

typedef struct {
    char endpoint_url[256];
    char username[64];
    char password[64];
} OPCUAConfig;

// Structure for MessageBus config
typedef struct {
        char *protocol;
        char *host;
        uint16_t port;
        char *authmode;
        char *clientid;
        int qos;
        int keepalive;
        bool retained;
        char *certfile;
        char *keyfile;
        char *privateKey;
        bool skipverify;
        char *basetopicprefix;
        int buffer_msg;
        bool cleansession;
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

typedef struct {
    char device_name[64];
    OPCUAConfig opcua;
    MQTTConfig mqtt;
    DataPoint data_points[MAX_DATA_POINTS];
    int num_data_points;
    bool active;
} DeviceConfig;

extern DeviceConfig g_device_config;

typedef enum {
    TYPE_BOOL,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32,
    TYPE_UINT32,
    TYPE_INT64,
    TYPE_UINT64,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_DATETIME,
    TYPE_UNKNOWN
} ValueType;

typedef struct {
    char alias[64];
    ValueType type;
    bool ready;

    union {
        UA_Boolean v_bool;
        UA_Int16 v_int16;
        UA_UInt16 v_uint16;
        UA_Int32 v_int32;
        UA_UInt32 v_uint32;
        UA_Int64 v_int64;
        UA_UInt64 v_uint64;
        UA_Float v_float;
        UA_Double v_double;
        UA_String v_string;
        UA_DateTime v_datetime;
    } value;

} OPCUAValue;

#endif // DEVICE_CONFIG_H
