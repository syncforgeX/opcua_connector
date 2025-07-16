#include "device_config.h"
#include "rest_server.h"
#include "json_utils.h"
#include <cjson/cJSON.h>
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

struct ConnectionInfo {
  char *data;
  size_t size;
};

void save_device_config(cJSON *json_data) {
  cJSON *device_id = cJSON_GetObjectItem(json_data, "device_name");
  if (!device_id || !cJSON_IsString(device_id)) {
    log_error("[ERROR] Invalid device JSON format.\n");
    return;
  }

  char filename[256];
  snprintf(filename, sizeof(filename), "%s/%s.json",METADATA_DIR,
           device_id->valuestring);

  FILE *file = fopen(filename, "w");
  if (!file) {
    log_error("Failed to save device config for %s PATH %s\n",
              device_id->valuestring, filename);
    return;
  }

  char *json_string = cJSON_Print(json_data);
  if (json_string) {
    fprintf(file, "%s", json_string);
    free(json_string); // Free allocated memory
  }

  log_debug("Device Configuration written to: %s", filename);
  fclose(file);
}

static int handle_post_payload(const char *json) {
  cJSON *json_data = cJSON_Parse(json);
  if (json_data) {
    save_device_config(json_data);
    cJSON_Delete(json_data);
  } else {
    log_error("Invalid JSON received.\n");
  }

  syslog(LOG_INFO, "Received POST payload");
  return process_json_payload(json);
}

// Function to send response
enum MHD_Result send_response(struct MHD_Connection *connection,
                              const char *message) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(message), (void *)message, MHD_RESPMEM_MUST_COPY);
  enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}

enum MHD_Result handle_post_request(void *cls,
                                    struct MHD_Connection *connection,
                                    const char *url, const char *method,
                                    const char *version,
                                    const char *upload_data,
                                    size_t *upload_data_size, void **con_cls) {
  log_debug("handle_request called for URL: %s ", url);
  fflush(stdout);

  if (strcmp(method, "POST") == 0 && strcmp(url, "/deviceconfigure") == 0) {

    struct ConnectionInfo *con_info = *con_cls;

    if (*con_cls == NULL) {
      con_info = calloc(1, sizeof(struct ConnectionInfo));
      if (con_info == NULL)
        return MHD_NO;
      *con_cls = con_info;
      return MHD_YES;
    }

    if (*upload_data_size != 0) {
      log_debug("upload_data_size: %zu bytes", *upload_data_size);
      fflush(stdout);

      con_info->data =
          realloc(con_info->data, con_info->size + *upload_data_size + 1);
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
               "{\"status\": \"success\", \"message\": \" device Configuration "
               "updated successfully\"}");

    } else {
      snprintf(response_text, sizeof(response_text),
               "{\"status\": \"error\", \"message\": \"Device Configuration "
               "update failed\"}");
    }
    free(con_info->data);
    free(con_info);
    return send_response(connection, response_text);
  } else{
	log_error("User Post Request method ERROR: %s URL: %s ", method, url);
  }

  return MHD_NO;
}

enum MHD_Result handle_get_request(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    uint64_t *upload_data_size,
    void **con_cls)
{
    (void)cls;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    log_debug("[DEBUG] handle_request called for URL: %s\n", url);
    fflush(stdout);

    if (strcmp(method, "GET") != 0)
        return MHD_NO;

    // Only accept exact match: /deviceconfig
    if (strcmp(url, "/deviceconfig") != 0)
        return MHD_NO;

    DIR *dir = opendir(METADATA_DIR);
    if (!dir) {
        log_error("Failed to open metadata directory");
        const char *msg = "{\"error\":\"Failed to open metadata directory\"}";
        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
        MHD_destroy_response(resp);
        return ret;
    }

    char *result = malloc(GET_PAYLOAD_SIZE);
    if (!result) {
        closedir(dir);
        return MHD_NO;
    }

    strcpy(result, "[");
    int first = 1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".json")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", METADATA_DIR, entry->d_name);
	    log_debug("File present in directory %s",filepath);

            FILE *fp = fopen(filepath, "r");
            if (!fp) continue;

            char filebuf[4096];
            size_t len = fread(filebuf, 1, sizeof(filebuf) - 1, fp);
            fclose(fp);
            filebuf[len] = '\0';

            if (!first) strcat(result, ",");
            strncat(result, filebuf, GET_PAYLOAD_SIZE - strlen(result) - 1);
            first = 0;
        }
    }
    closedir(dir);
    strcat(result, "]");

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(result), result, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");

    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

enum MHD_Result handle_del_request(void *cls, struct MHD_Connection *connection,
                                   const char *url, const char *method,
                                   const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls) {
    (void)cls;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    log_debug("[DEBUG] handle_DELETE called for URL: %s\n", url);
    fflush(stdout);
    if (strcmp(method, "DELETE") != 0)
        return MHD_NO;

    const char *prefix = "/deviceconfig/";
    if (strncmp(url, prefix, strlen(prefix)) != 0)
        return MHD_NO;

    const char *device_name = url + strlen(prefix);
    if (strlen(device_name) == 0)
        return MHD_NO;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s.json", METADATA_DIR, device_name);

    if (access(path, F_OK) != 0) {
	log_error("Device config not found %s",path);
        const char *msg = "{\"error\":\"Device config not found\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg),
                                            (void *)msg, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }

    if (remove(path) != 0) {
        log_error("Failed to delete metadata file: %s", path);
        const char *msg = "{\"error\":\"Failed to delete device config\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg),
                                            (void *)msg, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    g_device_config.active = false;
    log_debug("Device config deleted and deactivate the Thread successfully %s ",path);

    const char *msg = "{\"status\":\"Device config deleted successfully\"}";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg),
                                        (void *)msg, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

