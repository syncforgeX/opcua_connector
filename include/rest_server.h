#ifndef REST_SERVER_H
#define REST_SERVER_H

#include "log_utils.h"
#include <microhttpd.h>
#define POST_PORT 8080

enum MHD_Result handle_post_request(void *cls,
                                    struct MHD_Connection *connection,
                                    const char *url, const char *method,
                                    const char *version,
                                    const char *upload_data,
                                    size_t *upload_data_size, void **con_cls);
/*int handle_get_request(void *cls, struct MHD_Connection *connection,
                       const char *url, const char *method,
                       const char *version, const char *upload_data,
                       size_t *upload_data_size, void **con_cls);

int handle_del_request(void *cls, struct MHD_Connection *connection,
                       const char *url, const char *method,
                       const char *version, const char *upload_data,
                       size_t *upload_data_size, void **con_cls);
*/
#endif
