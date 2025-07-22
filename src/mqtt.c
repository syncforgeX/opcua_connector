#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "log_utils.h"
#include "mqtt.h"
#include "mqtt_queue.h"
#include "device_config.h"

mqtt_data_st mqtt_data = { .json_payload = "", .json_ready = false };

//timer
static struct sigevent sev;
static struct itimerspec ts; 
static timer_t mqtt_timerid;
static volatile uint8_t mqtt_exp = INIT_VAL; // to count reconnection exp count
static pthread_mutex_t data_mutex; // Mutex for thread-safe access to the data
static pthread_t mqtt_tid;
static volatile char mqtt_flag = INIT_VAL; //based timer handler to set 1
volatile char file_flag ;

// MQTT client handle
static MQTTClient client;
extern mqtt_data_st mqtt_data;
extern volatile uint8_t DataDelay_cntr;

/**
 * @brief Publishes a message to the MQTT broker on a predefined topic.
 * @param message The message string to be published (null-terminated).
 * @return void
 */
void publish_message(const char *message) {
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        MQTTClient_deliveryToken token;

        pubmsg.payload = (void *)message;
        pubmsg.payloadlen = (int)strlen(message);
        pubmsg.qos = g_device_config.mqtt.qos;
        pubmsg.retained = 0;

        int rc = MQTTClient_publishMessage(client, g_device_config.mqtt.base_topic, &pubmsg, &token);
        if (rc != MQTTCLIENT_SUCCESS) {
                log_error("Failed to publish message: %s (Error: %d)", message, rc);
        } else {
                log_debug("Message published: %s", message);
                MQTTClient_waitForCompletion(client, token, TIMEOUT);
        }   
}

/**
 * @brief Attempts to reconnect to the MQTT broker if the connection is lost.
 * @return int Returns `MQTTCLIENT_SUCCESS` on success or an error code on failure.
 */
static int reconnect_mqtt() {
        char ret = CLEAR;

        if(mqtt_exp == RECONNECT_CNT){
                ret = mqtt_client_deinit();
                if(ret){
                        return ENOT_OK;
                }   

                ret = mqtt_client_init();
                if(ret){
                        mqtt_exp = CLEAR;
                        return ENOT_OK;
                }
                goto L1;
        }

        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;

        ret = MQTTClient_connect(client, &conn_opts);
        if (ret != MQTTCLIENT_SUCCESS) {
		log_error("Failed to reconnect to broker. Error code: %d", ret);
                mqtt_exp++;
                return ENOT_OK;
        }

L1:
        file_flag = SET;
        mqtt_exp = CLEAR;
        log_debug("Reconnected to MQTT broker at %s",g_device_config.mqtt.broker_url);
        return MQTTCLIENT_SUCCESS;
}

/**
 * @brief Timer handler to check connection status, dequeue messages, and publish to the MQTT broker.
 */
static void mqtt_timer_handler(union sigval arg) {
        mqtt_flag = SET;
}

/**
 * @brief Initializes the MQTT client, connects to the broker, and sets up a recurring timer for message handling.
 */
static bool mqtt_mutex_init() {
        // Initialize the mutex
        if (pthread_mutex_init(&data_mutex, NULL) != 0) {
                log_error("Error initializing mutex");
                return ENOT_OK;
        }
        return E_OK;
}

static bool mqtt_client_init() {
        char ret = CLEAR;

        // Initialize MQTT client
        MQTTClient_create(&client, g_device_config.mqtt.broker_url, g_device_config.mqtt.client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        //MQTTClient_create(&client, "tcp://localhost:1883", "LocalClient1234", MQTTCLIENT_PERSISTENCE_NONE, NULL);

        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;

        // Connect to the MQTT broker
        if ((ret = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
                log_error("Failed to connect to broker %s  %s client_id %s. Error code: %d",g_device_config.mqtt.broker_url, g_device_config.mqtt.client_id, ret);
                return ENOT_OK;
        }
        return E_OK;
}

static bool mqtt_timer_init() {
        // Set up a timer to call the mqtt_timer_handler every 5 seconds
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = mqtt_timer_handler;
        sev.sigev_notify_attributes = NULL;
        sev.sigev_value.sival_ptr = &mqtt_timerid;

        if (timer_create(CLOCK_REALTIME, &sev, &mqtt_timerid) == -1) {
                log_error("timer_create failed");
                return ENOT_OK;
        }

        ts.it_value.tv_sec = MQTT_TIMER_INITIAL_START;  // Initial timer set to 5 seconds before the first interrupt
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = MQTT_INTERVAL;  // Repeat every 5 seconds
        ts.it_interval.tv_nsec = 0;

        if (timer_settime(mqtt_timerid, 0, &ts, NULL) == -1) {
                log_error("timer_settime failed");
                return ENOT_OK;
        }
        log_debug("MQTT Timer init done\n");
        return E_OK;
}

/**
 * @brief mqtt thread to check connection status, dequeue messages, and publish to the MQTT broker every 5sec.
 */
static void *mqtt_thread(void *arg) {
        while(1){
                if(mqtt_flag){
                        pthread_mutex_lock(&data_mutex);                                // Lock the mutex before accessing shared data
                        char json_payload[MAX_JSON_SIZE];

                        if (!isEmpty(&queue)) {
                                if (MQTTClient_isConnected(client)) {                   // Check if the client is connected to the broker
                                        if (dequeue(&queue, json_payload)) {      // If connected, publish the message
						mqtt_data.json_ready = false;                   // Reset the flag after sending
						goto E1;
                                        }
                                        publish_message(json_payload);
                                        mqtt_data.json_ready = false;                   // Reset the flag after sending
                                } else {                                                // Attempt to reconnect if the client is not connected
                                        if (reconnect_mqtt() == MQTTCLIENT_SUCCESS) {   // If reconnection is successful, publish the message
                                                if (dequeue(&queue, json_payload)) {
							mqtt_data.json_ready = false;                   // Reset the flag after sending
							goto E1;
                                                }
                                                publish_message(json_payload);
                                                mqtt_data.json_ready = false;           // Reset the flag after sending
                                        } else {
                                                log_debug("Unable to reconnect to MQTT broker. Retrying...\n");
                                        }
                                }
                        } else {
                                log_debug("JSON payload not ready isEmpty . Retrying...\n");
                        }
E1:
                        mqtt_flag = CLEAR;
                        pthread_mutex_unlock(&data_mutex); // Unlock the mutex
                }
        }
}
static bool mqtt_thread_init(){
        if (pthread_create(&mqtt_tid, NULL, mqtt_thread,NULL) != 0) {
                log_error("Failed to create MQTT thread");
                return ENOT_OK;
        }
        log_debug("MQTT Thread init done\n");
        return E_OK;
}

uint8_t mqtt_init() {
        char ret = 0 ;
        // Initialize mqtt queue
        MqttQueue_init(&queue);
        // Split initialization tasks
        ret = mqtt_mutex_init();
        if(ret){
                return ENOT_OK;
        }

        ret = mqtt_client_init();
        if(ret){
        //      return ENOT_OK;
        }

        ret = mqtt_timer_init();
        if(ret){
                return ENOT_OK;
        }

        ret = mqtt_thread_init();
        if(ret){
                return ENOT_OK;
        }
        return E_OK;
}

// Timer deinitialization
static void mqtt_timer_deinit() {
        if (timer_delete(mqtt_timerid) == -1) {
                log_error("Failed to delete timer");
        } else {
                log_debug("Timer deleted successfully.\n");
        }
}

// Deinitialize MQTT client
static bool  mqtt_client_deinit() {
        char ret;

        // Disconnect from the MQTT broker
        if ((ret = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS) {
                log_error("Failed to disconnect from broker. Error code: %d", ret);
                return ENOT_OK;
        } else {
                log_debug("Disconnected from broker.");
        }

        // Destroy the MQTT client
        MQTTClient_destroy(&client);

        log_debug("MQTT client destroyed.\n");
        return E_OK;
}

// Deinitialize the mutex
static void mqtt_mutex_deinit() {
        if (pthread_mutex_destroy(&data_mutex) != 0) {
                log_error("Error destroying mutex");
                // Handle error if needed
        } else {
                log_debug("Mutex destroyed successfully.");
        }
}

bool mqtt_deinit(){
        // Deinitialize the mutex before exiting
        mqtt_mutex_deinit();
        // Deinitialize MQTT client before exiting
        bool ret = mqtt_client_deinit();
        // Deinitialize the MQTT timer before exiting
        mqtt_timer_deinit();

}

// Function to build the JSON payload
static void build_opcua_json_payload(OPCUAValue *g_opcua_values, char *json_payload) {
    char entry[512];
    strcpy(json_payload, "[");  // Start of JSON array

    for (int i = 0; i < g_device_config.num_data_points; ++i) {
        OPCUAValue *val = &g_opcua_values[i];
        if (!val->ready)
            continue;

        // Get current UTC timestamp
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm *tm_info = gmtime(&ts.tv_sec);
        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ts.tv_nsec / 1000000);

        // Determine type string and value string
        const char *type_str = "unknown";
        char value_str[256] = {0};

        switch (val->type) {
            case TYPE_BOOL:
                type_str = "bool";
                snprintf(value_str, sizeof(value_str), "%s", val->value.v_bool ? "true" : "false");
                break;
            case TYPE_INT16:
                type_str = "int16";
                snprintf(value_str, sizeof(value_str), "%d", val->value.v_int16);
                break;
            case TYPE_UINT16:
                type_str = "uint16";
                snprintf(value_str, sizeof(value_str), "%u", val->value.v_uint16);
                break;
            case TYPE_INT32:
                type_str = "int32";
                snprintf(value_str, sizeof(value_str), "%d", val->value.v_int32);
                break;
            case TYPE_UINT32:
                type_str = "uint32";
                snprintf(value_str, sizeof(value_str), "%u", val->value.v_uint32);
                break;
            case TYPE_INT64:
                type_str = "int64";
                snprintf(value_str, sizeof(value_str), "%lld", (long long)val->value.v_int64);
                break;
            case TYPE_UINT64:
                type_str = "uint64";
                snprintf(value_str, sizeof(value_str), "%llu", (unsigned long long)val->value.v_uint64);
                break;
            case TYPE_FLOAT:
                type_str = "float";
                snprintf(value_str, sizeof(value_str), "%.3f", val->value.v_float);
                break;
            case TYPE_DOUBLE:
                type_str = "double";
                snprintf(value_str, sizeof(value_str), "%.6f", val->value.v_double);
                break;
            case TYPE_STRING:
                type_str = "string";
                snprintf(value_str, sizeof(value_str), "\"%s\"", (const char *)val->value.v_string.data);
                break;
            case TYPE_DATETIME:
                type_str = "datetime";
                snprintf(value_str, sizeof(value_str), "\"<datetime>\"");  // Format UA_DateTime if needed
                break;
            default:
                snprintf(value_str, sizeof(value_str), "\"unsupported\"");
                break;
        }

        // Construct JSON entry
        snprintf(entry, sizeof(entry),
                 "{\"tag\":\"%s\",\"type\":\"%s\",\"value\":%s,\"timestamp\":\"%s\",\"quality\":\"good\"}",
                 val->alias, type_str, value_str, timestamp);

        strcat(json_payload, entry);
        if (i < g_device_config.num_data_points - 1)
            strcat(json_payload, ",");
    }

    strcat(json_payload, "]");
}

// Function to handle data enqueueing with delay check
static uint8_t enqueue_data(char *json_payload) {
	if (enqueue(&queue, json_payload) == 1) {
		log_error("ERR: Enqueue failed...");
		return ENOT_OK;
	}

	mqtt_data.json_ready = true;
	return E_OK;
}

/**
 * @brief Collects machine data, formats it as a JSON payload, and enqueues it for publishing.
 * @param machn_data Pointer to a `machine_data_end` structure containing the machine data to be collected.
 * @return void
 */
uint8_t data_collection(OPCUAValue *g_opcua_values) {

        // Build the JSON payload
        build_opcua_json_payload(g_opcua_values, mqtt_data.json_payload);

//	log_debug("json payload %s",mqtt_data.json_payload);
         //Enqueue data if delay condition is met
        if(enqueue_data(mqtt_data.json_payload)){
                return ENOT_OK;
        }

        return E_OK;
}

