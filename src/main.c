#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include "rest_server.h"

static volatile int running = 1;

void handle_signal(int sig) {
    running = 0;
}

int main() {
	openlog("iot_connector", LOG_PID | LOG_CONS, LOG_USER);
	log_info("Starting IoT connector server...");
	signal(SIGINT, handle_signal);

	struct MHD_Daemon *daemon_post = MHD_start_daemon(
			MHD_USE_SELECT_INTERNALLY, POST_PORT, NULL, NULL,
			&handle_post_request, NULL, MHD_OPTION_END);
	if (!daemon_post) {
		syslog(LOG_ERR, "Failed to start POST server");
		return 1;
	}
	log_info("POST server running on port %d", POST_PORT);
/*
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
*/
	// Run until Ctrl+C
	while (running) {
		sleep(1);
	}

	log_info("Shutting down servers...");
	MHD_stop_daemon(daemon_post);
//	MHD_stop_daemon(daemon_get);
//	MHD_stop_daemon(daemon_del);

	closelog();
	return 0;
}

