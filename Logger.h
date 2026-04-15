//
// Created by mete on 15.04.2026.
//

#ifndef HTTP_SERVER_LOGGER_H
#define HTTP_SERVER_LOGGER_H

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
} LogLevel;

struct Logger
{
    LogLevel min_level;
    FILE *output;
    pthread_mutex_t lock;
    int use_color;
};

// Global logger instance
extern struct Logger g_logger;

void logger_init(struct Logger *logger, LogLevel min_level, FILE *output, int use_color);
void logger_destroy(struct Logger *logger);

void log_msg(struct Logger *logger, LogLevel level, const char *file, int line,
             const char *fmt, ...);

// Convenience macros
#define LOG_DEBUG(...) log_msg(&g_logger, LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_msg(&g_logger, LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_msg(&g_logger, LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_msg(&g_logger, LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

const char* log_level_str(LogLevel level);

#endif //HTTP_SERVER_LOGGER_H
