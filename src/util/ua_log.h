#ifndef UA_LOG_H_
#define UA_LOG_H_

/**
   @defgroup logging Logging

   @brief Logging functionality is externally provided to the open62541 libary.
   That is, every thread that wants logging to take place fills a global
   variable "logger" with the appropriate callback functions. The UA_LOGLEVEL
   preprocessor definition indicates which severity of events (trance, debug,
   info, warning, error, fatal) shall be reported. Enabling logs at a certain
   level enables all logs at the higher levels also. Furthermore, every
   log-message has a category that can be used for filtering or to select the
   output medium (file, stdout, ..).
*/

typedef enum UA_LoggerCategory {
	UA_LOGGERCATEGORY_CONNECTION,
	UA_LOGGERCATEGORY_SESSION,
	UA_LOGGERCATEGORY_SUBSCRIPTION,
	UA_LOGGERCATEGORY_MAINTENANCE,
	UA_LOGGERCATEGORY_LOAD,
} UA_LoggerCategory;

typedef struct UA_Logger {
	void (*log_trace)(UA_LoggerCategory category, const char *msg, ...);
	void (*log_debug)(UA_LoggerCategory category, const char *msg, ...);
	void (*log_info)(UA_LoggerCategory category, const char *msg, ...);
	void (*log_warning)(UA_LoggerCategory category, const char *msg, ...);
	void (*log_error)(UA_LoggerCategory category, const char *msg, ...);
	void (*log_fatal)(UA_LoggerCategory category, const char *msg, ...);
} UA_Logger;

/** The logger is a global variable on the stack. So every thread needs to
	initialise its own logger. */
static UA_Logger logger;

#if UA_LOGLEVEL <= 100
#define LOG_TRACE(CATEGORY, MSG, ...) do{ logger.log_trace(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define LOG_TRACE(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 200
#define LOG_DEBUG(CATEGORY, MSG, ...) do{ logger.log_debug(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define LOG_DEBUG(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 300
#define LOG_INFO(CATEGORY, MSG, ...) do{ logger.log_info(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define LOG_INFO(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 400
#define LOG_WARNING(CATEGORY, MSG, ...) do{ logger.log_warning(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define LOG_WARNING(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 500
#define LOG_ERROR(CATEGORY, MSG, ...) do{ logger.log_error(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define LOG_ERROR(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 600
#define LOG_FATAL(CATEGORY, MSG, ...) do{ logger.log_fatal(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define LOG_FATAL(CATEGORY, MSG, ...) do {} while(0)
#endif

#endif /* UA_LOG_H_ */
