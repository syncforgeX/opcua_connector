#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include "device_config.h"
#include "log_utils.h"
#include "opcua_client.h"
#include "mqtt.h" 

#define MAX_RECONNECT_ATTEMPTS 5

static pthread_t opcua_thread;
static bool tid_sts;

static timer_t opcua_timerid;                           // Timer ID
static pthread_t opcua_tid;                             //thread ID

static volatile uint8_t timer_flag = INIT_VAL;           // Flag to be monitored
static volatile uint8_t timer_count = INIT_VAL;         // Timer count variable
volatile uint8_t DataDelay_cntr = INIT_VAL;             // Timer count variable

static void log_opcua_values(OPCUAValue *g_opcua_values) {
	for (int i = 0; i < g_device_config.num_data_points; ++i) {
		OPCUAValue *val = &g_opcua_values[i];
		if (!val->ready)
			continue;  // Skip if data is not ready

		switch (val->type) {
			case TYPE_BOOL:
				log_debug("Alias='%s' → Boolean=%s", val->alias, val->value.v_bool ? "true" : "false");
				break;
			case TYPE_INT16:
				log_debug("Alias='%s' → Int16=%d", val->alias, val->value.v_int16);
				break;
			case TYPE_UINT16:
				log_debug("Alias='%s' → UInt16=%u", val->alias, val->value.v_uint16);
				break;
			case TYPE_INT32:
				log_debug("Alias='%s' → Int32=%d", val->alias, val->value.v_int32);
				break;
			case TYPE_UINT32:
				log_debug("Alias='%s' → UInt32=%u", val->alias, val->value.v_uint32);
				break;
			case TYPE_INT64:
				log_debug("Alias='%s' → Int64=%" PRId64, val->alias, val->value.v_int64);
				break;
			case TYPE_UINT64:
				log_debug("Alias='%s' → UInt64=%" PRIu64, val->alias, val->value.v_uint64);
				break;
			case TYPE_FLOAT:
				log_debug("Alias='%s' → Float=%.2f", val->alias, val->value.v_float);
				break;
			case TYPE_DOUBLE:
				log_debug("Alias='%s' → Double=%.2f", val->alias, val->value.v_double);
				break;
			case TYPE_STRING:
				log_debug("Alias='%s' → String='%.*s'", val->alias,
						(int)val->value.v_string.length, (const char *)val->value.v_string.data);
				break;
			case TYPE_DATETIME: {
						    UA_DateTimeStruct dts = UA_DateTime_toStruct(val->value.v_datetime);
						    log_debug("Alias='%s' → DateTime=%04u-%02u-%02u %02u:%02u:%02u",
								    val->alias, dts.year, dts.month, dts.day, dts.hour, dts.min, dts.sec);
						    break;
					    }
			case TYPE_UNKNOWN:
			default:
					    log_warn("Alias='%s' → Unknown or unsupported type", val->alias);
					    break;
		}
	}
	log_debug("\n");
}

bool opcua_read_values(UA_Client *client, OPCUAValue *out_values, size_t max_points) {
	bool any_data_ready = false;

	for (int i = 0; i < max_points; ++i) {
		DataPoint *dp = &g_device_config.data_points[i];
		OPCUAValue *val_out = &out_values[i];
		strncpy(val_out->alias, dp->alias, sizeof(val_out->alias) - 1);
		val_out->ready = false;

		UA_Variant value;
		UA_Variant_init(&value);
		UA_NodeId nodeId = UA_NODEID_NUMERIC(dp->namespace, dp->identifier);

		UA_StatusCode read_status = UA_Client_readValueAttribute(client, nodeId, &value);

		if (read_status == UA_STATUSCODE_GOOD && value.type && value.data) {
			if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
				val_out->type = TYPE_BOOL;
				val_out->value.v_bool = *(UA_Boolean *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
				val_out->type = TYPE_INT16;
				val_out->value.v_int16 = *(UA_Int16 *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
				val_out->type = TYPE_UINT16;
				val_out->value.v_uint16 = *(UA_UInt16 *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
				val_out->type = TYPE_INT32;
				val_out->value.v_int32 = *(UA_Int32 *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
				val_out->type = TYPE_UINT32;
				val_out->value.v_uint32 = *(UA_UInt32 *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT64])) {
				val_out->type = TYPE_INT64;
				val_out->value.v_int64 = *(UA_Int64 *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64])) {
				val_out->type = TYPE_UINT64;
				val_out->value.v_uint64 = *(UA_UInt64 *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
				val_out->type = TYPE_FLOAT;
				val_out->value.v_float = *(UA_Float *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
				val_out->type = TYPE_DOUBLE;
				val_out->value.v_double = *(UA_Double *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
				val_out->type = TYPE_STRING;
				val_out->value.v_string = *(UA_String *)value.data;
			} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
				val_out->type = TYPE_DATETIME;
				val_out->value.v_datetime = *(UA_DateTime *)value.data;
			} else {
				val_out->type = TYPE_UNKNOWN;
			}

			val_out->ready = true;
			any_data_ready = true;

		} else {
			log_warn("Failed to read value from node %d:%d — Status: %s",
					dp->namespace, dp->identifier, UA_StatusCode_name(read_status));
			val_out->type = TYPE_UNKNOWN;
			val_out->ready = false;

			if (read_status == UA_STATUSCODE_BADSESSIONCLOSED ||
					read_status == UA_STATUSCODE_BADCOMMUNICATIONERROR) {
				log_error("Session closed or communication error. Will reconnect next loop.");
				UA_Client_disconnect(client);
				UA_Client_delete(client);
				return false;
			}
		}

		UA_Variant_clear(&value);
	}

	return any_data_ready;
}

UA_Client *opcua_connect() {
	UA_Client *client = UA_Client_new();
	UA_ClientConfig_setDefault(UA_Client_getConfig(client));
	log_info("Attempting to connect to OPC UA server: %s", g_device_config.opcua.endpoint_url);

	UA_StatusCode status = UA_Client_connect(client, g_device_config.opcua.endpoint_url);
	if (status != UA_STATUSCODE_GOOD) {
		log_error("Connection failed: %s", UA_StatusCode_name(status));
		UA_Client_delete(client);
		return NULL;
	}

	log_info("Connected to OPC UA server successfully");
	return client;
}

static void *opcua_client_thread(void *arg) {
	log_info("OPC UA thread started...");
	bool *tid_sts = (bool *)arg;
	UA_Client *client = NULL;
	OPCUAValue g_opcua_values[MAX_DATA_POINTS];

	while (*tid_sts) {
		if (!g_device_config.active) {
			log_debug("OPC UA inactive, waiting...");
			if (client) {
				UA_Client_disconnect(client);
				UA_Client_delete(client);
				client = NULL;
			}
			// Clear opcua and data point related fields to prevent garbage use
			memset(&g_device_config.opcua, 0, sizeof(OPCUAConfig));
			memset(g_device_config.data_points, 0, sizeof(g_device_config.data_points));
			g_device_config.num_data_points = 0;
			sleep(1);
			continue;
		}

		// (1) CONNECT OR RECONNECT
		if (!client) {
			client = opcua_connect();
			if (!client) {
				sleep(1);
				continue;
			}
		}

		// (2) READ VALUES
		bool success = opcua_read_values(client, g_opcua_values, g_device_config.num_data_points);
		if (success) {
			if (data_collection(g_opcua_values)) {
				log_error("Failed to process OPC UA values");
			}
		} else {
			log_debug("No valid OPC UA values read. Skipping data collection.");
		}

		sleep(MQTT_INTERVAL); // Polling interval
	}

	if (client) {
		UA_Client_disconnect(client);
		UA_Client_delete(client);
	}

	log_debug("OPCUA_Client Thread ended.");
	return NULL;
}

/*
static void *opcua_client_thread(void *arg) {
	OPCUAValue g_opcua_values[MAX_DATA_POINTS];
	log_info("OPC UA thread started...");
	bool *tid_sts = (bool *)arg;
	UA_Client *client = NULL;
	UA_StatusCode status;
	while (*tid_sts) {
		if (!g_device_config.active) {
			log_debug("OPC UA inactive, waiting...");
			sleep(1);
			continue;
		}

		// (1) CONNECT OR RECONNECT
		if (!client) {
			client = UA_Client_new();
			UA_ClientConfig_setDefault(UA_Client_getConfig(client));
			log_info("Attempting to connect to OPC UA server: %s", g_device_config.opcua.endpoint_url);
			status = UA_Client_connect(client, g_device_config.opcua.endpoint_url);

			if (status != UA_STATUSCODE_GOOD) {
				log_error("Connection failed: %s", UA_StatusCode_name(status));
				UA_Client_delete(client);
				client = NULL;
				sleep(1);
				continue;
			}
			log_info("Connected to OPC UA server successfully");
		}

		bool any_data_ready = false;
		// (2) READ VALUES
		for (int i = 0; i < g_device_config.num_data_points; ++i) {
			DataPoint *dp = &g_device_config.data_points[i];
			OPCUAValue *val_out = &g_opcua_values[i];
			strncpy(val_out->alias, dp->alias, sizeof(val_out->alias) - 1);
			val_out->ready = false;

			UA_Variant value;
			UA_Variant_init(&value);

			UA_NodeId nodeId = UA_NODEID_NUMERIC(dp->namespace, dp->identifier);
			UA_StatusCode read_status = UA_Client_readValueAttribute(client, nodeId, &value);

			if (read_status == UA_STATUSCODE_GOOD && value.type && value.data) {
				// [same type-handling logic as your code...]
				if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
					val_out->type = TYPE_BOOL;
					val_out->value.v_bool = *(UA_Boolean *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
					val_out->type = TYPE_INT16;
					val_out->value.v_int16 = *(UA_Int16 *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
					val_out->type = TYPE_UINT16;
					val_out->value.v_uint16 = *(UA_UInt16 *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
					val_out->type = TYPE_INT32;
					val_out->value.v_int32 = *(UA_Int32 *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
					val_out->type = TYPE_UINT32;
					val_out->value.v_uint32 = *(UA_UInt32 *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT64])) {
					val_out->type = TYPE_INT64;
					val_out->value.v_int64 = *(UA_Int64 *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64])) {
					val_out->type = TYPE_UINT64;
					val_out->value.v_uint64 = *(UA_UInt64 *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
					val_out->type = TYPE_FLOAT;
					val_out->value.v_float = *(UA_Float *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
					val_out->type = TYPE_DOUBLE;
					val_out->value.v_double = *(UA_Double *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
					val_out->type = TYPE_STRING;
					val_out->value.v_string = *(UA_String *)value.data;
				} else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
					val_out->type = TYPE_DATETIME;
					val_out->value.v_datetime = *(UA_DateTime *)value.data;
				} else {
					val_out->type = TYPE_UNKNOWN;
				}

				val_out->ready = true;
				any_data_ready = true;
			} else {
				log_warn("Failed to read value from node %d:%d — Status: %s",
						dp->namespace, dp->identifier, UA_StatusCode_name(read_status));
				val_out->type = TYPE_UNKNOWN;
				val_out->ready = false;

				if (read_status == UA_STATUSCODE_BADSESSIONCLOSED ||
						read_status == UA_STATUSCODE_BADCOMMUNICATIONERROR) {
					log_error("Session closed or communication error. Will reconnect next loop.");
					UA_Client_disconnect(client);
					UA_Client_delete(client);
					client = NULL;
					break; // Exit for loop and go to next while iteration (reconnect)
				}
			}

			UA_Variant_clear(&value);
		}

		// (3) Process Collected Values
		if (any_data_ready) {
			if (data_collection(g_opcua_values)) {
				log_error("Failed to process OPC UA values");
			}
		} else {
			log_debug("No valid OPC UA values read. Skipping data collection.");
		}

		sleep(MQTT_INTERVAL); // Polling rate
	}

	// Cleanup
	if (client) {
		UA_Client_disconnect(client);
		UA_Client_delete(client);
	}

	log_debug("OPCUA_Client Thread ended.");
	return NULL;
}
*/

// Timer handler function
static void opcua_timer_handler(union sigval sv) {
        timer_count++;                                  // Increment the timer count every 1 second
        DataDelay_cntr++;                               // free running counter for data delay  
        // printf("Timer callback: Timer count = %d\n", timer_count);
  /*                                                      // If the timer count completes 1 second, set the timer_flag
        if (timer_count >= 1) {
                if(!timer_flag)                         //if timer is 0 then only flag change to 1 poll start
                        timer_flag = SET;               // Set the timer_flag

                timer_count = CLEAR;                    // Reset the timer count for the next cycle
        }*/
}

// Initialization function
static bool opcua_timer_init() {
        struct sigevent sev;
        struct itimerspec its;

        // Configure the timer event to call the timer_handler function
        sev.sigev_notify = SIGEV_THREAD;                // Notify via a thread
        sev.sigev_value.sival_ptr = &opcua_timerid;     // Pass timer ID to the handler
        sev.sigev_notify_function = opcua_timer_handler;// Timer handler function
        sev.sigev_notify_attributes = NULL;             // Default thread attributes

        // Create the timer
        if (timer_create(CLOCK_REALTIME, &sev, &opcua_timerid) == -1) {
                perror("Failed to create timer");
                return ENOT_OK;
        }

        // Configure the timer: 1-second interval
        its.it_value.tv_sec = OPCUA_TIMER_INITIAL_START;// Initial expiration in seconds
        its.it_value.tv_nsec = 0;                       // Initial expiration in nanoseconds
        its.it_interval.tv_sec = 1;        // Periodic interval in seconds
        its.it_interval.tv_nsec = 0;                    // Periodic interval in nanoseconds

        // Start the timer
        if (timer_settime(opcua_timerid, 0, &its, NULL) == -1) {
                perror("Failed to start timer");
                return ENOT_OK;
        }

        log_debug("Timer initialized and started.");
        return E_OK;
}

uint8_t opcua_init(){
        int8_t ret = CLEAR;

        tid_sts = 1;
        // OPC UA Worker Thread
        if (pthread_create(&opcua_thread, NULL, opcua_client_thread, &tid_sts) != 0) {
                log_error("Failed to create OPC UA thread");
                return ENOT_OK;
        }

        ret = opcua_timer_init(&opcua_timerid); // Initialize the timer
        if(ret){
                return ENOT_OK;
        }
        log_debug("opcua timer initializion successfully");

        return E_OK;
}

// Timer deinitialization function
static void opcua_timer_deinit() {
        // Check if timer exists and delete it
        if (opcua_timerid) {
                if (timer_delete(opcua_timerid) == -1) {
                        log_error("Failed to delete opcua timer");
                } else {
                        log_debug("opcua timer deleted successfully.");
                }
        } else {
                log_debug("No valid opcua timer to delete.");
        }
}

// Full deinitialization function
void opcua_deinit() {

        // Notify thread to stop (if needed)
        tid_sts = 0;
        // Wait for thread to exit
        pthread_join(opcua_thread, NULL);

        opcua_timer_deinit();     // Deinitialize the timer
        log_info("opcua deinitialized successfully.");
}
