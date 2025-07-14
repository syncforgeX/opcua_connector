#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H
#define MAX_DATA_POINTS 50

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
} DeviceConfig;

extern DeviceConfig g_device_config;

#endif // DEVICE_CONFIG_H
