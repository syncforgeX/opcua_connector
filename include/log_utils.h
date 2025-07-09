#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <syslog.h>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3


// Allow setting from -DCURRENT_LOG_LEVEL=LOG_LEVEL_INFO, etc.
#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG //Default Log Level
#endif

#define log_debug(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG) syslog(LOG_DEBUG, fmt, ##__VA_ARGS__); } while (0)

#define log_info(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO) syslog(LOG_INFO, fmt, ##__VA_ARGS__); } while (0)

#define log_warn(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_WARN) syslog(LOG_WARNING, fmt, ##__VA_ARGS__); } while (0)

#define log_error(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR) syslog(LOG_ERR, fmt, ##__VA_ARGS__); } while (0)

/*
#define log_info(fmt, ...)    syslog(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...)   syslog(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)    syslog(LOG_WARNING, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)   syslog(LOG_ERR, fmt, ##__VA_ARGS__)
*/

#endif
