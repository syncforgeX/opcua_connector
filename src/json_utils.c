#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <syslog.h>
#include "device_config.h"
#include "log_utils.h"
#include "mqtt_client.h"

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
    log_debug("OPCUA Password     : %s", config->opcua.password); // Mask if needed

    const MessageBusConfig *mb = &config->mqtt.messagebus;

    log_debug("MQTT Protocol      : %s", mb->protocol);
    log_debug("MQTT Host          : %s", mb->host);
    log_debug("MQTT Port          : %u", mb->port);
    log_debug("MQTT Auth Mode     : %s", mb->authmode);
    log_debug("MQTT Client ID     : %s", mb->clientid);
    log_debug("MQTT QOS           : %d", mb->qos);
    log_debug("MQTT Keepalive     : %d", mb->keepalive);
    log_debug("MQTT Retained      : %s", mb->retained ? "true" : "false");
    log_debug("MQTT Clean Session : %s", mb->cleansession ? "true" : "false");
    log_debug("MQTT Skip Verify   : %s", mb->skipverify ? "true" : "false");
    log_debug("MQTT Base Topic    : %s", mb->basetopicprefix);
    log_debug("MQTT Buffer Msg    : %d", mb->buffer_msg);
    log_debug("MQTT Cert File     : %s", mb->certfile);
    log_debug("MQTT Key File      : %s", mb->keyfile);
    log_debug("MQTT Private Key   : %s", mb->privateKey);

    log_debug("Data Points (%d):", config->num_data_points);
    for (int i = 0; i < config->num_data_points; i++) {
        const DataPoint *dp = &config->data_points[i];
        log_debug("  [%d] NodeID: ns=%d;i=%d (%s), Type: %s, Alias: %s",
                  i,
                  dp->namespace,
                  dp->identifier,
                  dp->nodeid,
                  dp->datatype,
                  dp->alias);
    }
}

int process_json_payload(const char *json) {
	cJSON *root = cJSON_Parse(json);
	if (!root) {
		log_error("INVALID JSON");
		return -1;
	}

	log_debug("Parsed JSON:\n%s", json);

	cJSON *device_name = cJSON_GetObjectItem(root, "device_name");
	cJSON *opcua = cJSON_GetObjectItem(root, "opcua");
	cJSON *msgbus = cJSON_GetObjectItem(root, "messagebus");
	cJSON *data_points = cJSON_GetObjectItem(root, "data_points");

	if (!cJSON_IsString(device_name) || !cJSON_IsObject(opcua) ||
			!cJSON_IsObject(msgbus) || !cJSON_IsArray(data_points)) {
		syslog(LOG_ERR, "Missing or invalid main fields");
		cJSON_Delete(root);
		return -1;
	}

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

	// MessageBus (MQTT)
	MessageBusConfig *m = &g_device_config.mqtt.messagebus;
#define COPY_STRING_FIELD(obj, key, target) \
	do { \
		cJSON *tmp = cJSON_GetObjectItem(obj, key); \
		if (cJSON_IsString(tmp)) target = strdup(tmp->valuestring); \
	} while (0)

	COPY_STRING_FIELD(msgbus, "protocol", m->protocol);
	COPY_STRING_FIELD(msgbus, "host", m->host);
	COPY_STRING_FIELD(msgbus, "authmode", m->authmode);
	COPY_STRING_FIELD(msgbus, "clientid", m->clientid);
	COPY_STRING_FIELD(msgbus, "certfile", m->certfile);
	COPY_STRING_FIELD(msgbus, "keyfile", m->keyfile);
	COPY_STRING_FIELD(msgbus, "privateKey", m->privateKey);
	COPY_STRING_FIELD(msgbus, "basetopicprefix", m->basetopicprefix);

	cJSON *port = cJSON_GetObjectItem(msgbus, "port");
	if (cJSON_IsNumber(port)) m->port = (uint16_t)port->valueint;

	cJSON *qos = cJSON_GetObjectItem(msgbus, "qos");
	if (cJSON_IsNumber(qos)) m->qos = qos->valueint;

	cJSON *keepalive = cJSON_GetObjectItem(msgbus, "keepalive");
	if (cJSON_IsNumber(keepalive)) m->keepalive = keepalive->valueint;

	cJSON *retained = cJSON_GetObjectItem(msgbus, "retained");
	if (cJSON_IsBool(retained)) m->retained = retained->valueint;

	cJSON *skipverify = cJSON_GetObjectItem(msgbus, "skipverify");
	if (cJSON_IsBool(skipverify)) m->skipverify = skipverify->valueint;

	cJSON *cleansession = cJSON_GetObjectItem(msgbus, "cleansession");
	if (cJSON_IsBool(cleansession)) m->cleansession = cleansession->valueint;

	cJSON *buffer_msg = cJSON_GetObjectItem(msgbus, "buffer_msg");
	if (cJSON_IsNumber(buffer_msg)) m->buffer_msg = buffer_msg->valueint;

#undef COPY_STRING_FIELD

	// Parse data_points
	int count = cJSON_GetArraySize(data_points);
	g_device_config.num_data_points = 0;

	for (int i = 0; i < count && i < MAX_DATA_POINTS; ++i) {
		cJSON *item = cJSON_GetArrayItem(data_points, i);
		if (!cJSON_IsObject(item)) continue;

		cJSON *ns = cJSON_GetObjectItem(item, "namespace");
		cJSON *id = cJSON_GetObjectItem(item, "identifier");
		cJSON *datatype = cJSON_GetObjectItem(item, "datatype");
		cJSON *alias = cJSON_GetObjectItem(item, "alias");

		if (cJSON_IsNumber(ns) && cJSON_IsNumber(id) &&
				cJSON_IsString(datatype) && cJSON_IsString(alias)) {

			g_device_config.data_points[i].namespace = ns->valueint;
			g_device_config.data_points[i].identifier = id->valueint;

			snprintf(g_device_config.data_points[i].nodeid,
					sizeof(g_device_config.data_points[i].nodeid),
					"ns=%d;i=%d", ns->valueint, id->valueint);

			strncpy(g_device_config.data_points[i].datatype, datatype->valuestring,
					sizeof(g_device_config.data_points[i].datatype) - 1);
			strncpy(g_device_config.data_points[i].alias, alias->valuestring,
					sizeof(g_device_config.data_points[i].alias) - 1);

			g_device_config.num_data_points++;
		}
	}

	log_device_config(&g_device_config);
	cJSON_Delete(root);
	g_device_config.active = true;
	log_debug("Device getting Active");

	return 0;
}

