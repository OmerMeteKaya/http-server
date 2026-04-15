//
// Created by mete on 15.04.2026.
//

#include "Logger.h"
#include <time.h>
#include <string.h>
#include <pthread.h>

struct Logger g_logger;

void logger_init(struct Logger *logger, LogLevel min_level, FILE *output, int use_color)
{
    logger->min_level = min_level;
    logger->output = output;
    logger->use_color = use_color;
    pthread_mutex_init(&logger->lock, NULL);
}

void logger_destroy(struct Logger *logger)
{
    pthread_mutex_destroy(&logger->lock);
}

const char* log_level_str(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

static const char* log_color(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "\033[36m";  // Cyan
        case LOG_INFO:  return "\033[32m";  // Green
        case LOG_WARN:  return "\033[33m";  // Yellow
        case LOG_ERROR: return "\033[31m";  // Red
        default:        return "\033[0m";
    }
}

#define COLOR_RESET "\033[0m"

void log_msg(struct Logger *logger, LogLevel level, const char *file, int line,
             const char *fmt, ...)
{
    if (level < logger->min_level) return;

    pthread_mutex_lock(&logger->lock);

    // Timestamp
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);

    // Thread ID
    pthread_t tid = pthread_self();
    unsigned long thread_id = (unsigned long)(tid);

    // Color prefix if enabled
    if (logger->use_color) {
        fprintf(logger->output, "%s[%s]%s [%s] [tid=%lu] [%s:%d] ",
                log_color(level), timestamp, COLOR_RESET,
                log_level_str(level), thread_id, file, line);
    } else {
        fprintf(logger->output, "[%s] [%s] [tid=%lu] [%s:%d] ",
                timestamp, log_level_str(level), thread_id, file, line);
    }

    // User message
    va_list args;
    va_start(args, fmt);
    vfprintf(logger->output, fmt, args);
    va_end(args);

    fprintf(logger->output, "\n");
    fflush(logger->output);

    pthread_mutex_unlock(&logger->lock);
}
