#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <dirent.h>
#include "opcua_client.h"
#include "json_utils.h"
#include "mqtt_client.h"
#include "rest_server.h"
#include "device_config.h"

static volatile int running = 1;

static void handle_signal(int sig) { running = 0; }

static void load_device_config() {
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

pthread_t mqtt_tid;
static bool tid_sts = true;
edgex_bus_t *bus = NULL;

static void *mqtt_client_thread(void *arg) {

	while (tid_sts) {
		if (!g_device_config.active) {
			log_debug("MQTT inactive, waiting...");

			if (bus) {
				bus->freefn(bus);
				bus = NULL;
			}

			sleep(1);
			continue;
		}

		if (!bus) {
			bus = mqtt_client_init(&g_device_config, "test-service");
			if (bus) {
				log_error("MQTT init failed, retrying...");
				sleep(2);
				continue;
			}

			log_info("MQTT initialized successfully.");
		}

		usleep(100);
	}

	if (bus) {
		bus->freefn(bus);
		bus = NULL;
	}

	log_info("MQTT thread exited.");
	return NULL;
}

int mqtt_init() {

	tid_sts = true;
	if(pthread_create(&mqtt_tid, NULL, mqtt_client_thread, NULL) != 0 ) {
		log_error("Failed to create MQTT thread");
		return ENOT_OK;
	}
	return E_OK;
}

int main() {
	char ret = 0;
	openlog("iot_connector", LOG_PID | LOG_CONS, LOG_USER);
	log_info("Starting IoT connector server...");
	signal(SIGINT, handle_signal);

	load_device_config();  // Might not load anything

	ret = init_http_servers();  // Start REST servers
	if (ret) {
		log_error("Failed to start http Server.");
		return 0;
	}
	log_info("Starting http Server...");

	ret = mqtt_init();
	if (ret) {
		log_error("Failed to start MQTT thread.");
	}
	log_info("Starting MQTT thread...");

	ret = opcua_init();
	if (ret) {
		log_error("Failed to start OPC UA thread.");
	}
	log_info("Starting OPC UA thread...");

	while (running) {
		sleep(2);
	}

	log_info("Shutting down servers...");
	deinit_http_servers();
	tid_sts = false;
	opcua_deinit();
	closelog();
	return 0;
}

