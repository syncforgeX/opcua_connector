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
    edgex_bus_t *bus = (edgex_bus_t *)context;
    MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;

    pthread_mutex_lock(&cinfo->mtx);
    cinfo->connected = true;
    pthread_cond_signal(&cinfo->cond);
    pthread_mutex_unlock(&cinfo->mtx);

    printf("✅ MQTT connected\n");
}

static void mqtt_on_connect_failure(void *context, MQTTAsync_failureData *response) {
    edgex_bus_t *bus = (edgex_bus_t *)context;
    MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;

    pthread_mutex_lock(&cinfo->mtx);
    cinfo->connected = false;
    pthread_cond_signal(&cinfo->cond);
    pthread_mutex_unlock(&cinfo->mtx);

    printf("❌ MQTT connection failed\n");
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
    fprintf(stderr, "❌ MQTT publish failed, rc=%d\n", response ? response->code : -1);
}

bool edgex_bus_mqtt_post(edgex_bus_t *bus, const char *topic, const char *msg)
{
    if (!bus || !topic || !msg) {
        fprintf(stderr, "Invalid arguments to edgex_bus_mqtt_post()\n");
        return false;
    }

    MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    pubmsg.payload = (void *)msg;
    pubmsg.payloadlen = (int)strlen(msg);
    pubmsg.qos = 0;
    pubmsg.retained = 0;
    opts.context = cinfo;
    opts.onSuccess = edgex_bus_mqtt_onsend;
    opts.onFailure = edgex_bus_mqtt_onsendfail;

    printf("📤 Publishing to topic [%s] message: %s\n", topic, msg);

    if (MQTTAsync_isConnected(cinfo->client)) {
        int rc = MQTTAsync_sendMessage(cinfo->client, topic, &pubmsg, &opts);
        if (rc == MQTTASYNC_SUCCESS) {
            printf("✅ Publish request sent\n");
        } else {
            printf("❌ MQTT publish failed, rc=%d\n", rc);
        }
    } else {
        printf("❌ MQTT client not fully connected, skipping publish\n");
    }

    return true;
}

void edgex_bus_mqtt_free(edgex_bus_t *bus) {
    MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;
    if (cinfo) {
        MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
        MQTTAsync_disconnect(cinfo->client, &disc_opts);
        MQTTAsync_destroy(&cinfo->client);
        free(cinfo->uri);
        free(bus);
    }
}

bool edgex_bus_mqtt_subscribe(edgex_bus_t *bus, const char *topic)
{
    if (!bus || !bus->ctx || !topic) return false;

    MessageBusConfig *cinfo = (MessageBusConfig *)bus->ctx;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTSubscribe_options sub_opts = MQTTSubscribe_options_initializer;

    sub_opts.noLocal = 1;  // Optional: don't receive own publishes
    opts.context = cinfo;
    opts.subscribeOptions = sub_opts;

    int rc = MQTTAsync_subscribe(cinfo->client, topic, 1, &opts);
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

#define CA_CERT     "./certs/cacert.pem"
#define CLIENT_CERT "./certs/clientcert.pem"
#define CLIENT_KEY  "./certs/clientkey.pem"

edgex_bus_t *mqtt_client_init(DeviceConfig *config, const char *svcname) {
    int rc;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;

    edgex_bus_t *bus = calloc(1, sizeof(edgex_bus_t));
    if (!bus) return NULL;

    MessageBusConfig *cinfo = &config->mqtt.messagebus;
    bus->ctx = cinfo;

    const char *prot = (cinfo->protocol && strlen(cinfo->protocol)) ? cinfo->protocol : "tcp";
    if (strcmp(prot, "mqtt") == 0 || strcmp(prot, "tcp") == 0) {
        prot = "tcp";
    } else if (strcmp(prot, "ssl") == 0 || strcmp(prot, "tls") == 0 ||
               strcmp(prot, "mqtts") == 0 || strcmp(prot, "mqtt+ssl") == 0 ||
               strcmp(prot, "tcps") == 0) {
        prot = "mqtts";
    } else {
        fprintf(stderr, "Unsupported MQTT protocol: %s\n", prot);
        free(bus);
        return NULL;
    }

    if (cinfo->port == 0)
        cinfo->port = (strcmp(prot, "ssl") == 0) ? 8883 : 1883;

    size_t urisize = strlen(cinfo->host) + 32;
    cinfo->uri = malloc(urisize);
    snprintf(cinfo->uri, urisize, "%s://%s:%d", prot, cinfo->host, cinfo->port);
    create_opts.sendWhileDisconnected = 1;
 
    rc = MQTTAsync_createWithOptions(&cinfo->client, cinfo->uri, cinfo->clientid,
                                     MQTTCLIENT_PERSISTENCE_NONE, NULL, &create_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        fprintf(stderr, "Failed to create MQTT client: %d\n", rc);
        free(cinfo->uri);
        free(bus);
        return NULL;
    }

    pthread_mutex_init(&cinfo->mtx, NULL);
    pthread_cond_init(&cinfo->cond, NULL);
    cinfo->connected = false;

    conn_opts.keepAliveInterval = 1;
    conn_opts.cleansession = 1;
    conn_opts.automaticReconnect = 1;
    conn_opts.onSuccess = mqtt_on_connect;
    conn_opts.onFailure = mqtt_on_connect_failure;
    conn_opts.context = bus;
	conn_opts.maxInflight = 100;
/*
    if (strcmp(prot, "ssl") == 0) {
        conn_opts.ssl = &ssl_opts;
        if (cinfo->certfile) ssl_opts.trustStore = cinfo->certfile;
        if (cinfo->keyfile) ssl_opts.keyStore = cinfo->keyfile;
        ssl_opts.verify = 1;
    }
*/
	    ssl_opts.trustStore = CA_CERT;
            ssl_opts.keyStore = CLIENT_CERT;
            ssl_opts.privateKey = CLIENT_KEY;
	    conn_opts.ssl = &ssl_opts;
    MQTTAsync_setCallbacks(cinfo->client, bus, NULL, edgex_bus_mqtt_msgarrvd, NULL);

    for (int i = 0; i < MQTT_CONNECT_RETRIES; ++i) {
        pthread_mutex_lock(&cinfo->mtx);

        rc = MQTTAsync_connect(cinfo->client, &conn_opts);
        if (rc == MQTTASYNC_SUCCESS) {
	    printf("dobne \n");
            pthread_cond_wait(&cinfo->cond, &cinfo->mtx);
        }

        pthread_mutex_unlock(&cinfo->mtx);

        if (cinfo->connected) {
	
		break;
	}
        usleep(MQTT_RETRY_DELAY_MS * 1000);
    }

    if (!cinfo->connected) {
        MQTTAsync_destroy(&cinfo->client);
        free(cinfo->uri);
        pthread_mutex_destroy(&cinfo->mtx);
        pthread_cond_destroy(&cinfo->cond);
        free(bus);
        return NULL;
    }

    // Initialize the edgex_bus interface
    edgex_bus_init(bus, svcname, config);
    bus->postfn = edgex_bus_mqtt_post;
    bus->freefn = edgex_bus_mqtt_free;
    bus->subsfn = edgex_bus_mqtt_subscribe;

    return bus;
}

