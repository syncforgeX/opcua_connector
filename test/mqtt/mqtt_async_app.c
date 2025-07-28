// gcc -DTEST_MAIN mqtt_async_app.c -o mqtt_async_app -lpaho-mqtt3as -lpthread

#include <MQTTAsync.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define ADDRESS     "mqtts://10.20.32.132:8883"
#define CLIENTID    "AsyncClientID1i1"
#define TOPIC       "sensor/data"
#define QOS         1
#define TIMEOUT     10000L

#define CA_CERT     "./certs/cacert.pem"
#define CLIENT_CERT "./certs/clientcert.pem"
#define CLIENT_KEY  "./certs/clientkey.pem"

static MQTTAsync client;
static volatile bool mqtt_connected = false;
volatile uint8_t mqtt_flag = 0;

static pthread_t mqtt_thread;

// --------------------- Callbacks ---------------------
static void on_mqtt_connect(void *context, MQTTAsync_successData *response) {
    printf("✅ MQTTAsync connected.\n");
    mqtt_connected = true;
}

static void on_mqtt_connect_failure(void *context, MQTTAsync_failureData *response) {
    printf("❌ MQTTAsync connection failed.\n");
    mqtt_connected = false;
}

static void on_mqtt_connection_lost(void *context, char *cause) {
    printf("⚠️ MQTTAsync connection lost: %s\n", cause);
    mqtt_connected = false;
}

// --------------------- Reconnect Loop ---------------------
static void *mqtt_thread_fn(void *arg) {
    static int reconnect_delay_sec = 1;  // Initial backoff delay

    while (1) {
        if (!mqtt_connected) {
            printf("🔁 Trying to reconnect in %d sec...\n", reconnect_delay_sec);
            sleep(1);  // Delay before reconnect attempt

            MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
            MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;

            ssl_opts.trustStore = CA_CERT;
            ssl_opts.keyStore = CLIENT_CERT;
            ssl_opts.privateKey = CLIENT_KEY;
            ssl_opts.enableServerCertAuth = 1;

            conn_opts.automaticReconnect = 0;  // we handle it manually
            conn_opts.keepAliveInterval = 1;
            conn_opts.cleansession = 1;
            conn_opts.ssl = &ssl_opts;
            conn_opts.onSuccess = on_mqtt_connect;
            conn_opts.onFailure = on_mqtt_connect_failure;
            conn_opts.context = client;

            int rc = MQTTAsync_connect(client, &conn_opts);
            if (rc != MQTTASYNC_SUCCESS) {
                printf("❌ MQTTAsync_connect failed to start: %d\n", rc);
            }

            // Increase reconnect delay (max 10s)
            if (reconnect_delay_sec < 10)
                reconnect_delay_sec++;
        }

        if (mqtt_connected && mqtt_flag) {
            mqtt_flag = 0;

            const char *payload = "data from device";

            MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
            MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

            pubmsg.payload = (void *)payload;
            pubmsg.payloadlen = strlen(payload);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;

            int rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts);
            if (rc != MQTTASYNC_SUCCESS) {
                if (rc == MQTTASYNC_DISCONNECTED) {
                    mqtt_connected = false;
                }
                printf("❌ Failed to publish: %d\n", rc);
            } else {
                printf("✅ Published to %s: %s\n", TOPIC, payload);
            }

            // Reset reconnect delay on success
            reconnect_delay_sec = 1;
        }

        usleep(100000); // 100ms loop
    }

    return NULL;
}

// --------------------- Initialization ---------------------
bool mqtt_client_init() {
    int rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("❌ MQTTAsync_create failed: %d\n", rc);
        return false;
    }

    MQTTAsync_setCallbacks(client, NULL, on_mqtt_connection_lost, NULL, NULL);

    rc = pthread_create(&mqtt_thread, NULL, mqtt_thread_fn, NULL);
    if (rc != 0) {
        printf("❌ Failed to create MQTT thread.\n");
        return false;
    }

    return true;
}

// --------------------- Deinit ---------------------
bool mqtt_client_deinit() {
    if (mqtt_connected) {
        MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
        disc_opts.timeout = 1000;
        MQTTAsync_disconnect(client, &disc_opts);
    }

    MQTTAsync_destroy(&client);
    mqtt_connected = false;

    printf("✅ MQTTAsync disconnected.\n");
    return true;
}

// --------------------- Timer handler ---------------------
static void mqtt_timer_handler(union sigval arg) {
    mqtt_flag = 1;
}

int mqtt_timer_init() {
    timer_t timerid;
    struct sigevent sev = {
        .sigev_notify = SIGEV_THREAD,
        .sigev_notify_function = mqtt_timer_handler,
        .sigev_value.sival_ptr = &timerid
    };

    struct itimerspec its = {
        .it_interval.tv_sec = 1,
        .it_value.tv_sec = 1
    };

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) != 0) {
        perror("timer_create");
        return -1;
    }

    if (timer_settime(timerid, 0, &its, NULL) != 0) {
        perror("timer_settime");
        return -1;
    }

    return 0;
}

// --------------------- Full Entry Point ---------------------
int mqtt_init() {
    if (!mqtt_client_init()) return -1;
    if (mqtt_timer_init() != 0) return -1;
    return 0;
}

int mqtt_deinit() {
    return mqtt_client_deinit();
}

// --------------------- Main (For Test Only) ---------------------
#ifdef TEST_MAIN
int main() {
    if (mqtt_init() == 0) {
        printf("✅ MQTT subsystem initialized.\n");
    }

    while(1){
    
    }  // Let it run for a while

    mqtt_deinit();
    return 0;
}
#endif

