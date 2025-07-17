#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#define MAX_DATA_POINTS 50

#include <stdbool.h>
#include <open62541/client.h>

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

typedef struct {
    char broker_url[256];
    char username[64];
    char password[64];
    int publish_interval_ms;
    char base_topic[128];
    int tls_enabled;
    char certificate_path[256];
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
