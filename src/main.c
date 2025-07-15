#include "rest_server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "json_utils.h"

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

	// Run until Ctrl+C
	while (running) {
		sleep(1);
	}

	log_info("Shutting down servers...");
	MHD_stop_daemon(daemon_post);
	MHD_stop_daemon(daemon_get);
	MHD_stop_daemon(daemon_del);

	closelog();
	return 0;
}
