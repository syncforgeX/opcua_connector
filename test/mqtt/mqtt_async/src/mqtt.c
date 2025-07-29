#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <MQTTAsync.h>
#include "mqtt_client.h"

#define MQTT_CONNECT_RETRIES 10
#define MQTT_RETRY_DELAY_MS 500

static void mqtt_on_connect(void *context, MQTTAsync_successData *response) {
	MessageBusConfig *cinfo = (MessageBusConfig *)context;

	pthread_mutex_lock(&cinfo->mtx);
	cinfo->connected = true;
	pthread_cond_signal(&cinfo->cond);
	pthread_mutex_unlock(&cinfo->mtx);

	printf("✅ MQTT connected\n");
}

static void mqtt_on_connect_failure(void *context, MQTTAsync_failureData *response) {
	MessageBusConfig *cinfo = (MessageBusConfig *)context;

	pthread_mutex_lock(&cinfo->mtx);
	cinfo->connected = false;
	pthread_cond_signal(&cinfo->cond);
	pthread_mutex_unlock(&cinfo->mtx);
	if (response->message)
	{
		printf("mqtt: connect failed: %s (code %d)", response->message, response->code);
	}
	else
	{
		printf("mqtt: connect failed, error code %d", response->code);
	}

}

static int edgex_bus_mqtt_msgarrvd(void *context, char *topic, int topic_len, MQTTAsync_message *message) {
	printf("📩 Received message on topic %s: %.*s\n", topic, message->payloadlen, (char *)message->payload);
	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topic);
	return 1;
}

static void edgex_bus_mqtt_onsend(void *context, MQTTAsync_successData *response) {
	printf("✅ MQTT message published successfully\n");
}

static void edgex_bus_mqtt_onsendfail(void *context, MQTTAsync_failureData *response) {	
	MessageBusConfig *cinfo = (MessageBusConfig *)context;
	if (response->message)
	{
		printf("mqtt: publish failed: %s (code %d)", response->message, response->code);
	}
	else
	{
		printf("mqtt: publish failed, error code %d", response->code);
	}
}

bool edgex_bus_mqtt_post(void *ctx, const char *topic, const char *msg)
{
	edgex_bus_t *bus = (edgex_bus_t *)ctx;
	MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;

	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

	pubmsg.payload = (void *)msg;
	pubmsg.payloadlen = (int)strlen(msg);
	pubmsg.qos = cinfo->qos;
	pubmsg.retained = cinfo->retained;

	opts.context = cinfo;
	opts.onSuccess = edgex_bus_mqtt_onsend;
	opts.onFailure = edgex_bus_mqtt_onsendfail;

	printf("📤 Publishing to topic [%s] message: %s\n", topic, msg);

	int rc = MQTTAsync_sendMessage(cinfo->client, topic, &pubmsg, &opts);

	if (rc == MQTTASYNC_SUCCESS) {
		printf("✅ Publish request sent\n");
	} else if (rc == MQTTASYNC_DISCONNECTED) {
		printf("📦 Client disconnected — message buffered (if buffering enabled)\n");
	} else if (rc == MQTTASYNC_MAX_BUFFERED) {
		printf("⚠️ Message buffer full — consider increasing maxBufferedMessages\n");
	} else {
		printf("❌ MQTT publish failed, rc = %d\n", rc);
	}

	return true;
}

void edgex_bus_mqtt_free(void *ctx) {
	edgex_bus_t *bus = (edgex_bus_t *)ctx;
	MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;
	if (cinfo) {
		MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
		disc_opts.context = cinfo;
		MQTTAsync_disconnect(cinfo->client, &disc_opts);
		MQTTAsync_destroy(&cinfo->client);

		// Clean up threading primitives
		pthread_cond_destroy(&cinfo->cond);
		pthread_mutex_destroy(&cinfo->mtx);

		free(cinfo->uri);
		free(ctx);
	}
}

bool edgex_bus_mqtt_subscribe(void *ctx, const char *topic)
{

	MessageBusConfig *cinfo = (MessageBusConfig *)ctx;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTSubscribe_options sub_opts = MQTTSubscribe_options_initializer;

	sub_opts.noLocal = 1;  // Optional: don't receive own publishes
	opts.context = cinfo;
	opts.subscribeOptions = sub_opts;

	int rc = MQTTAsync_subscribe(cinfo->client, topic, cinfo->qos, &opts);
	if (rc != MQTTASYNC_SUCCESS) {
		fprintf(stderr, "❌ MQTT: subscribe to %s failed (rc=%d)\n", topic, rc);
		return false;
	}

	// Optional: Wait for subscription completion
	rc = MQTTAsync_waitForCompletion(cinfo->client, opts.token, 1000);
	if (rc != MQTTASYNC_SUCCESS) {
		fprintf(stderr, "❌ MQTT: waitForCompletion for %s failed (rc=%d)\n", topic, rc);
		return false;
	}

	printf("✅ MQTT: subscribed to topic %s\n", topic);
	return true;
}

void edgex_bus_init(edgex_bus_t *bus, const char *svcname, DeviceConfig *cfg) {
	// Placeholder if you want service name logic
	(void)svcname;
	(void)cfg;
}


edgex_bus_t *mqtt_client_init(DeviceConfig *config, const char *svcname) {
	int rc;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;
	edgex_bus_t *result;

	MessageBusConfig *cinfo = &config->mqtt.messagebus;

	const char *prot = (cinfo->protocol && strlen(cinfo->protocol)) ? cinfo->protocol : "tcp";
	if (*prot == '\0' || strcmp(prot, "mqtt") == 0 || strcmp(prot, "tcp") == 0) {
		prot = "tcp";
	} else if (strcmp(prot, "ssl") == 0 || strcmp(prot, "tls") == 0 ||
			strcmp(prot, "mqtts") == 0 || strcmp(prot, "mqtt+ssl") == 0 ||
			strcmp(prot, "tcps") == 0) {
		prot = "ssl";
	} else {
		fprintf(stderr, "Unsupported MQTT protocol: %s\n", prot);
		return NULL;
	}

	if (cinfo->port == 0){
		cinfo->port = (strcmp(prot, "ssl") == 0) ? 8883 : 1883;
	}

	size_t urisize = strlen(cinfo->host) + 32;
	cinfo->uri = malloc(urisize);
	snprintf(cinfo->uri, urisize, "%s://%s:%d", prot, cinfo->host, cinfo->port);

	create_opts.sendWhileDisconnected = 1;
	create_opts.maxBufferedMessages = cinfo->buffer_msg;  // ✅ number of messages to store in buffer

	rc = MQTTAsync_createWithOptions(&cinfo->client, cinfo->uri, cinfo->clientid,
			MQTTCLIENT_PERSISTENCE_DEFAULT, NULL, &create_opts);
	if (rc != MQTTASYNC_SUCCESS) {
		fprintf(stderr, "Failed to create MQTT client: %d\n", rc);
		free(cinfo->uri);
		return NULL;
	}

	result = malloc (sizeof (edgex_bus_t));
	MQTTAsync_setCallbacks(cinfo->client, result, NULL, edgex_bus_mqtt_msgarrvd, NULL);

	conn_opts.keepAliveInterval = cinfo->keepalive;
	conn_opts.cleansession = cinfo->cleansession;
	conn_opts.automaticReconnect = 1;
	conn_opts.onSuccess = mqtt_on_connect;
	conn_opts.onFailure = mqtt_on_connect_failure;
	conn_opts.context = cinfo;
	conn_opts.maxInflight = 100;

	if (strcmp(prot, "ssl") == 0) {
		conn_opts.ssl = &ssl_opts;
		if (cinfo->certfile) ssl_opts.trustStore = cinfo->certfile;
		if (cinfo->keyfile) ssl_opts.keyStore = cinfo->keyfile;
		if (cinfo->privateKey) ssl_opts.privateKey = cinfo->privateKey;
		ssl_opts.verify = cinfo->skipverify;
	}

	/*
	   if (strcmp (iot_data_string_map_get_string (cfg, EX_BUS_AUTHMODE), "usernamepassword") == 0)
	   {
	   secrets = edgex_secrets_get (secstore, iot_data_string_map_get_string (cfg, EX_BUS_SECRETNAME));
	   conn_opts.username = iot_data_string_map_get_string (secrets, "username");
	   conn_opts.password = iot_data_string_map_get_string (secrets, "password");
	   }*/
	pthread_mutex_init(&cinfo->mtx, NULL);
	pthread_cond_init(&cinfo->cond, NULL);
	cinfo->connected = false;

	for (int i = 0; i < MQTT_CONNECT_RETRIES; ++i) {
		pthread_mutex_lock(&cinfo->mtx);

		rc = MQTTAsync_connect(cinfo->client, &conn_opts);
		if (rc == MQTTASYNC_SUCCESS) {
			printf("dobne  %s \n", cinfo->uri);
			pthread_cond_wait(&cinfo->cond, &cinfo->mtx);
		}

		pthread_mutex_unlock(&cinfo->mtx);

		if (cinfo->connected) {
			break;
		}
		usleep(MQTT_RETRY_DELAY_MS * 1000);
	}

	pthread_mutex_destroy(&cinfo->mtx);
	pthread_cond_destroy(&cinfo->cond);

	if (cinfo->connected)
	{
		edgex_bus_init (result, svcname, NULL);
		result->ctx = cinfo;
		result->postfn = edgex_bus_mqtt_post;
		result->freefn = edgex_bus_mqtt_free;
		result->subsfn = edgex_bus_mqtt_subscribe;
	}else {
		free (cinfo->uri);
		free (result);
		result = NULL;
		//MQTTAsync_destroy(&cinfo->client);
	}

	return result;
}

