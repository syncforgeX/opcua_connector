#ifndef REST_SERVER_H
#define REST_SERVER_H

#include <microhttpd.h>

#define POST_PORT 8080
#define GET_PORT 8081
#define DEL_PORT 8082

#define METADATA_DIR "/etc/opcua_connector/metadata"
#define MAX_PATH 100
#define GET_PAYLOAD_SIZE 8192


static enum MHD_Result handle_post_request(void *cls,
                                    struct MHD_Connection *connection,
                                    const char *url, const char *method,
                                    const char *version,
                                    const char *upload_data,
                                    size_t *upload_data_size, void **con_cls);
static enum MHD_Result handle_get_request(void *cls,
                                   struct MHD_Connection *connection,
                                   const char *url,
                                   const char *method,
                                   const char *version,
                                   const char *upload_data,
                                   size_t *upload_data_size,
                                   void **con_cls);
static enum MHD_Result handle_del_request(void *cls, struct MHD_Connection *connection,
                const char *url, const char *method,
                const char *version, const char *upload_data,
                size_t *upload_data_size, void **con_cls);

void deinit_http_servers();
int init_http_servers();
#endif
