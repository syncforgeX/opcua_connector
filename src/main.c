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
#include "mqtt.h"
#include "rest_server.h"

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

int main() {
	char ret = 0;
	openlog("iot_connector", LOG_PID | LOG_CONS, LOG_USER);
	log_info("Starting IoT connector server...");
	signal(SIGINT, handle_signal);

	load_device_config();  // <<< Call this before HTTP starts

	ret = init_http_servers();
	if (ret) {
		return 0;
	}

	ret = opcua_init(); // Create opcua_tid
	if(ret){
		goto D_INIT;
	}
	log_info("opcua initializion successfully");

	ret = mqtt_init();// Initialize mqtt
	if(ret){
		goto D_INIT;
	}
	log_info("mqtt initialization is successfully\n");

	// Run until Ctrl+C
	while (running) {
		sleep(1);
	}

D_INIT:
	log_info("Shutting down servers...");
	deinit_http_servers();
	opcua_deinit();
	mqtt_deinit();
	closelog();
	return 0;
}
