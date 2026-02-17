#pragma once

#define MIN_LOG_LEVEL LVL_VERBOSE

typedef enum {
    LVL_VERBOSE = 0,
    LVL_DEBUG = 1,
    LVL_INFO = 2,
    LVL_WARN = 3,
    LVL_ERROR = 4,
    LVL_CRITICAL = 5
} DebugLevel;

void logf(const char* module, DebugLevel level, const char* fmt, ...);
#define log_verbose(module, ...) logf(module, LVL_VERBOSE, __VA_ARGS__)
#define log_debug(module, ...) logf(module, LVL_DEBUG, __VA_ARGS__)
#define log_info(module, ...) logf(module, LVL_INFO, __VA_ARGS__)
#define log_warn(module, ...) logf(module, LVL_WARN, __VA_ARGS__)
#define log_err(module, ...) logf(module, LVL_ERROR, __VA_ARGS__)
#define log_crit(module, ...) logf(module, LVL_CRITICAL, __VA_ARGS__)