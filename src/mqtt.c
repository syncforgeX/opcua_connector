#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "log_utils.h"
#include"mqtt.h"

mqtt_data_st mqtt_data = { .json_payload = "", .json_ready = false };


// Function to build the JSON payload
void build_opcua_json_payload(OPCUAValue *g_opcua_values, char *json_payload) {
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
/*static uint8_t enqueue_data(char *json_payload) {

        // Enqueue the data every 5 sec
        if (DataDelay_cntr == MQTT_INTERVAL) {
                if (enqueue(&queue, json_payload) == 1) {
                        printf("ERR: Enqueue failed...\n");
                        DataDelay_cntr = CLEAR;  // Clear the counter after enqueueing
                        //return 1;
                }
        DataDelay_cntr = CLEAR;  // Clear the counter after enqueueing
        }

        return E_OK;
}
*/
/**
 * @brief Collects machine data, formats it as a JSON payload, and enqueues it for publishing.
 * @param machn_data Pointer to a `machine_data_end` structure containing the machine data to be collected.
 * @return void
 */

// Updated data_collection function
uint8_t data_collection(OPCUAValue *g_opcua_values) {

        // Build the JSON payload
        build_opcua_json_payload(g_opcua_values, mqtt_data.json_payload);

        // Mark the payload as ready
        mqtt_data.json_ready = true;

	printf("json payload %s\n",mqtt_data.json_payload);
        // Enqueue data if delay condition is met
        /*if(enqueue_data(mqtt_data.json_payload)){
                return ENOT_OK;
        }
*/
        return E_OK;
}

