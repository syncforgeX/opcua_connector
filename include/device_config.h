#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

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
    char device_name[128];
    OPCUAConfig opcua;
    MQTTConfig mqtt;
} DeviceConfig;

extern DeviceConfig g_device_config;

#endif // DEVICE_CONFIG_H
