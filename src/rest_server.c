#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include "json_utils.h"
#include <stdio.h>
#include <cjson/cJSON.h>
#include "rest_server.h"

struct ConnectionInfo {
    char *data;
    size_t size;
};

static int handle_post_payload(const char *json) {
    syslog(LOG_INFO, "Received POST payload");
    return process_json_payload(json);
}

// Function to send response
enum MHD_Result send_response(struct MHD_Connection *connection, const char *message) {
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(message),
                        (void *)message,
                        MHD_RESPMEM_MUST_COPY);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
}

enum MHD_Result handle_post_request(
		void *cls,
		struct MHD_Connection *connection,
		const char *url,
		const char *method,
		const char *version,
		const char *upload_data,
		size_t *upload_data_size,
		void **con_cls)
{
	log_debug("handle_request called for URL: %s ", url);
	fflush(stdout);

	if (strcmp(method, "POST") == 0 && strcmp(url, "/deviceconfigure") == 0){

		struct ConnectionInfo *con_info = *con_cls;

		if (*con_cls == NULL) {
			con_info = calloc(1, sizeof(struct ConnectionInfo));
			if (con_info == NULL) return MHD_NO;
			*con_cls = con_info;
			return MHD_YES;
		}

		if (*upload_data_size != 0) {
			log_debug("upload_data_size: %zu bytes", *upload_data_size);
			fflush(stdout);

			con_info->data = realloc(con_info->data, con_info->size + *upload_data_size + 1);
			memcpy(con_info->data + con_info->size, upload_data, *upload_data_size);
			con_info->size += *upload_data_size;
			con_info->data[con_info->size] = '\0';

			*upload_data_size = 0; // Reset upload size to indicate data is processed
			return MHD_YES;
		}

		// Only process the request after all data is received
		log_debug("Full JSON Payload: %s", con_info->data);
		fflush(stdout);

		int result = handle_post_payload(con_info->data);

		// Prepare response
		char response_text[256];
		if (result == 0) {
			snprintf(response_text, sizeof(response_text),
					"{\"status\": \"success\", \"message\": \" device Configuration updated successfully\"}");

			cJSON *json_data = cJSON_Parse(con_info->data);
			if (json_data) {
				//save_taos_config(json_data);
				//cJSON_Delete(json_data);
			} else {
				printf( "[ERROR] Invalid config JSON received.\n");
			}

		} else {
			snprintf(response_text, sizeof(response_text),
					"{\"status\": \"error\", \"message\": \"Device Configuration update failed\"}");
		}
		free(con_info->data);
		free(con_info);
		return send_response(connection, response_text);
	}

	return MHD_NO;
}

