#include <MQTTAsync.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ADDRESS     "mqtts://10.20.32.132:8883"
#define CLIENTID    "AsyncClientID"
#define TOPIC       "sensor/data"
#define QOS         1
#define TIMEOUT     10000L

#define CA_CERT     "./certs/cacert.pem"
#define CLIENT_CERT "./certs/clientcert.pem"
#define CLIENT_KEY  "./certs/clientkey.pem"

static MQTTAsync client;
static volatile bool mqtt_connected = false;

// ----- Callback: on successful connection -----
static void on_mqtt_connect(void *context, MQTTAsync_successData *response) {
    printf("✅ MQTTAsync connected successfully.\n");
    mqtt_connected = true;
}

// ----- Callback: on connection failure -----
static void on_mqtt_connect_failure(void *context, MQTTAsync_failureData *response) {
    printf("❌ MQTTAsync connection failed.\n");
    mqtt_connected = false;
}

// ----- Callback: if connection lost -----
static void on_mqtt_connection_lost(void *context, char *cause) {
    printf("⚠️  MQTT connection lost: %s\n", cause);
    mqtt_connected = false;
}

// ----- MQTT Initialization -----
bool mqtt_client_init() {
    int rc;

    rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("❌ MQTTAsync_create failed: %d\n", rc);
        return false;
    }

    MQTTAsync_setCallbacks(client, NULL, on_mqtt_connection_lost, NULL, NULL);

    // TLS Options
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    ssl_opts.trustStore = CA_CERT;
    ssl_opts.keyStore = CLIENT_CERT;
    ssl_opts.privateKey = CLIENT_KEY;
    ssl_opts.enableServerCertAuth = 1;

    // Connection Options
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = on_mqtt_connect;
    conn_opts.onFailure = on_mqtt_connect_failure;
    conn_opts.context = client;
    conn_opts.ssl = &ssl_opts;

    rc = MQTTAsync_connect(client, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("❌ MQTTAsync_connect start failed: %d\n", rc);
        return false;
    }

    // Wait for connection to complete
    int wait_ms = 0;
    while (!mqtt_connected && wait_ms < 5000) {
        usleep(100000); // 100 ms
        wait_ms += 100;
    }

    return mqtt_connected;
}

// ----- Send a test message -----
void mqtt_send_test_message() {
    if (!mqtt_connected) {
        printf("❌ Cannot send, not connected.\n");
        return;
    }

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    pubmsg.payload = "hello async mqtt";
    pubmsg.payloadlen = (int)strlen(pubmsg.payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    int rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("❌ Failed to publish message: %d\n", rc);
    } else {
        printf("✅ Test message published to %s\n", TOPIC);
    }
}

// ----- Main function -----
int main() {
    if (mqtt_client_init()) {
        printf("✅ MQTT initialized and connected.\n");
        mqtt_send_test_message();
    } else {
        printf("❌ MQTT initialization failed.\n");
    }

    // Give time for message to be sent before disconnect
    sleep(2);

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_disconnect(client, &disc_opts);
    MQTTAsync_destroy(&client);

    return 0;
}

