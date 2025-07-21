#include "rest_server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "json_utils.h"
#include <pthread.h>
#include "opcua_client.h"
#include <stdbool.h>
#include "mqtt.h"

static volatile int running = 1;

void handle_signal(int sig) { running = 0; }

void load_device_config() {
    DIR *dir = opendir(METADATA_DIR);
    if (!dir) {
        log_error("Metadata directory not found: %s", METADATA_DIR);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
	    if (strstr(entry->d_name, ".json")) {
		    char filepath[MAX_PATH];
		    snprintf(filepath, sizeof(filepath), "%s/%s", METADATA_DIR, entry->d_name);
		    log_info("Loading config from: %s", filepath);

		    FILE *fp = fopen(filepath, "r");
		    if (!fp) {
			    log_error("Failed to open config file: %s", filepath);
			    continue;
		    }

		    fseek(fp, 0, SEEK_END);
		    long size = ftell(fp);
		    rewind(fp);

		    char *data = malloc(size + 1);
		    fread(data, 1, size, fp);
		    data[size] = '\0';
		    fclose(fp);

		    process_json_payload(data);//parse the config data in globallaly

		    free(data);
		    break; // Load only first config file for now
	    }
    }

    closedir(dir);
}

static timer_t opcua_timerid;                           // Timer ID
static pthread_t opcua_tid;                             //thread ID

static volatile uint8_t timer_flag = INIT_VAL;           // Flag to be monitored
static volatile uint8_t timer_count = INIT_VAL;         // Timer count variable
volatile uint8_t DataDelay_cntr = INIT_VAL;             // Timer count variable


// Timer handler function
static void opcua_timer_handler(union sigval sv) {
        timer_count++;                                  // Increment the timer count every 1 second
        DataDelay_cntr++;                               // free running counter for data delay  
        // printf("Timer callback: Timer count = %d\n", timer_count);

        // If the timer count completes 1 second, set the timer_flag
        if (timer_count >= 1) {
                if(!timer_flag)                         //if timer is 0 then only flag change to 1 poll start
                        timer_flag = SET;               // Set the timer_flag

                timer_count = CLEAR;                    // Reset the timer count for the next cycle
        }
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
        its.it_interval.tv_sec = OPCUA_INTERVAL;        // Periodic interval in seconds
        its.it_interval.tv_nsec = 0;                    // Periodic interval in nanoseconds

        // Start the timer
        if (timer_settime(opcua_timerid, 0, &its, NULL) == -1) {
                perror("Failed to start timer");
                return ENOT_OK;
        }

        printf("Timer initialized and started.\n");
        return E_OK;
}

static uint8_t opcua_init(){
        int8_t ret = CLEAR;
        ret = opcua_timer_init(&opcua_timerid); // Initialize the timer
        if(ret){
                return ENOT_OK;
        }
        printf("opcua timer initializion successfully\n");

        return E_OK;
}

// Timer deinitialization function
static void opcua_timer_deinit() {
    // Check if timer exists and delete it
    if (opcua_timerid) {
        if (timer_delete(opcua_timerid) == -1) {
            perror("Failed to delete opcua timer");
        } else {
            printf("Focas timer deleted successfully.\n");
        }
    } else {
        printf("No valid opcua timer to delete.\n");
    }
}

// Full deinitialization function
static void opcua_deinit() {
        opcua_timer_deinit();     // Deinitialize the timer
        printf("Focas deinitialized successfully.\n");
}

int main() {
	openlog("iot_connector", LOG_PID | LOG_CONS, LOG_USER);
	log_info("Starting IoT connector server...");
	signal(SIGINT, handle_signal);

	load_device_config();  // <<< Call this before HTTP starts

	struct MHD_Daemon *daemon_post =
		MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, POST_PORT, NULL, NULL,
				&handle_post_request, NULL, MHD_OPTION_END);
	if (!daemon_post) {
		syslog(LOG_ERR, "Failed to start POST server");
		return 1;
	}
	log_info("POST server running on port %d", POST_PORT);

	struct MHD_Daemon *daemon_get = MHD_start_daemon(
			MHD_USE_SELECT_INTERNALLY, GET_PORT, NULL, NULL,
			&handle_get_request, NULL, MHD_OPTION_END);
	if (!daemon_get) {
		syslog(LOG_ERR, "Failed to start GET server");
		MHD_stop_daemon(daemon_post);
		return 1;
	}
	log_info("GET server running on port %d", GET_PORT);

	struct MHD_Daemon *daemon_del = MHD_start_daemon(
			MHD_USE_SELECT_INTERNALLY, DEL_PORT, NULL, NULL,
			&handle_del_request, NULL, MHD_OPTION_END);
	if (!daemon_del) {
		syslog(LOG_ERR, "Failed to start DELETE server");
		MHD_stop_daemon(daemon_post);
		MHD_stop_daemon(daemon_get);
		return 1;
	}
	log_info("DELETE server running on port %d", DEL_PORT);

	// OPC UA Worker Thread
	pthread_t opcua_thread;
	bool tid_sts = 1;
	if (pthread_create(&opcua_thread, NULL, opcua_client_thread, &tid_sts) != 0) {
        log_error("Failed to create OPC UA thread");
        return 1;
	}

	char ret = 0;
        ret = opcua_init(); // Create opcua_tid
        if(ret){
                goto D_INIT;
        }

	printf("opcua initializion successfully\n");
	ret = mqtt_init();// Initialize mqtt
        if(ret){
                goto D_INIT;
        }
        log_debug("mqtt initialization is successfully\n");

	// Run until Ctrl+C
	while (running) {
		sleep(1);
	}

D_INIT:
	// Notify thread to stop (if needed)
	tid_sts = 0;
	// Wait for thread to exit
	pthread_join(opcua_thread, NULL);
	mqtt_deinit();
	opcua_deinit();
	log_info("Shutting down servers...");
	MHD_stop_daemon(daemon_post);
	MHD_stop_daemon(daemon_get);
	MHD_stop_daemon(daemon_del);

	closelog();
	return 0;
}
