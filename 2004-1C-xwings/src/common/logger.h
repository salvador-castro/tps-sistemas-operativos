#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum { LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_EVENT } LogLevel;

void log_init(const char *filename, const char *component_id);
void log_message(LogLevel level, const char *format, ...);
void log_close();

#endif // LOGGER_H
