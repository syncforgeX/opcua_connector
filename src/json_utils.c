#include "device_config.h"
#include "log_utils.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

DeviceConfig g_device_config = {0}; // Zero-initialize
                                    //
int write_cert_to_file(const char *path, const char *content) {
  FILE *fp = fopen(path, "w");
  if (!fp) {
    syslog(LOG_ERR, "Failed to open cert file: %s", path);
    return -1;
  }
  fputs(content, fp);
  fclose(fp);
  syslog(LOG_INFO, "Certificate written to: %s", path);
  return 0;
}

void log_device_config(const DeviceConfig *config) {
  log_debug("Device Name        : %s", config->device_name);

  log_debug("OPCUA Endpoint     : %s", config->opcua.endpoint_url);
  log_debug("OPCUA Username     : %s", config->opcua.username);
  log_debug("OPCUA Password     : %s",
            config->opcua.password); // Mask if needed

  log_debug("MQTT Broker URL    : %s", config->mqtt.broker_url);
  log_debug("MQTT Username      : %s", config->mqtt.username);
  log_debug("MQTT Password      : %s", config->mqtt.password); // Mask if needed
  log_debug("MQTT Interval (ms) : %d", config->mqtt.publish_interval_ms);
  log_debug("MQTT Base Topic    : %s", config->mqtt.base_topic);
  log_debug("MQTT TLS Enabled   : %s",
            config->mqtt.tls_enabled ? "true" : "false");
  log_debug("MQTT Cert Path     : %s", config->mqtt.certificate_path);
}

int process_json_payload(const char *json) {
  cJSON *root = cJSON_Parse(json);
  if (!root) {
    log_error("INVALID JSON");
    return -1;
  }

  cJSON *device_name = cJSON_GetObjectItem(root, "device_name");
  cJSON *opcua = cJSON_GetObjectItem(root, "opcua");
  cJSON *mqtt = cJSON_GetObjectItem(root, "mqtt");

  if (!cJSON_IsString(device_name) || !cJSON_IsObject(opcua) ||
      !cJSON_IsObject(mqtt)) {
    syslog(LOG_ERR, "Missing or invalid main fields");
    cJSON_Delete(root);
    return -1;
  }

  if (!cJSON_IsString(device_name))
    goto cleanup;
  strncpy(g_device_config.device_name, device_name->valuestring,
          sizeof(g_device_config.device_name) - 1);

  // OPC UA
  cJSON *endpoint_url = cJSON_GetObjectItem(opcua, "endpoint_url");
  cJSON *opc_user = cJSON_GetObjectItem(opcua, "username");
  cJSON *opc_pass = cJSON_GetObjectItem(opcua, "password");

  if (cJSON_IsString(endpoint_url))
    strncpy(g_device_config.opcua.endpoint_url, endpoint_url->valuestring,
            sizeof(g_device_config.opcua.endpoint_url) - 1);
  if (cJSON_IsString(opc_user))
    strncpy(g_device_config.opcua.username, opc_user->valuestring,
            sizeof(g_device_config.opcua.username) - 1);
  if (cJSON_IsString(opc_pass))
    strncpy(g_device_config.opcua.password, opc_pass->valuestring,
            sizeof(g_device_config.opcua.password) - 1);

  // MQTT
  cJSON *broker_url = cJSON_GetObjectItem(mqtt, "broker_url");
  cJSON *mqtt_user = cJSON_GetObjectItem(mqtt, "username");
  cJSON *mqtt_pass = cJSON_GetObjectItem(mqtt, "password");
  cJSON *interval = cJSON_GetObjectItem(mqtt, "publish_interval_ms");
  cJSON *base_topic = cJSON_GetObjectItem(mqtt, "base_topic");
  cJSON *tls_enabled = cJSON_GetObjectItem(mqtt, "tls_enabled");
  cJSON *cert_path = cJSON_GetObjectItem(mqtt, "certificate_path");
  cJSON *cert_content = cJSON_GetObjectItem(mqtt, "certificate_content");

  if (cJSON_IsString(broker_url))
    strncpy(g_device_config.mqtt.broker_url, broker_url->valuestring,
            sizeof(g_device_config.mqtt.broker_url) - 1);
  if (cJSON_IsString(mqtt_user))
    strncpy(g_device_config.mqtt.username, mqtt_user->valuestring,
            sizeof(g_device_config.mqtt.username) - 1);
  if (cJSON_IsString(mqtt_pass))
    strncpy(g_device_config.mqtt.password, mqtt_pass->valuestring,
            sizeof(g_device_config.mqtt.password) - 1);
  if (cJSON_IsNumber(interval))
    g_device_config.mqtt.publish_interval_ms = interval->valueint;
  if (cJSON_IsString(base_topic))
    strncpy(g_device_config.mqtt.base_topic, base_topic->valuestring,
            sizeof(g_device_config.mqtt.base_topic) - 1);
  if (cJSON_IsBool(tls_enabled))
    g_device_config.mqtt.tls_enabled = tls_enabled->valueint;
  if (cJSON_IsString(cert_path))
    strncpy(g_device_config.mqtt.certificate_path, cert_path->valuestring,
            sizeof(g_device_config.mqtt.certificate_path) - 1);

  // Write cert to file
  if (cJSON_IsString(cert_content)) {
    write_cert_to_file(g_device_config.mqtt.certificate_path,
                       cert_content->valuestring);
  }

  log_device_config(&g_device_config); // optional for debug log

  cJSON_Delete(root);
  return 0;

cleanup:
  cJSON_Delete(root);
  return -1;
}
