#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *log_file = NULL;
static char component_name[32] = "";

void log_init(const char *filename, const char *component_id) {
  log_file = fopen(filename, "a");
  if (log_file) {
    strncpy(component_name, component_id, sizeof(component_name) - 1);
    log_message(LOG_INFO, "Logger initialized for %s", component_id);
  }
}

void log_message(LogLevel level, const char *format, ...) {
  if (!log_file)
    return;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

  const char *level_str = "INFO";
  switch (level) {
  case LOG_WARNING:
    level_str = "WARNING";
    break;
  case LOG_ERROR:
    level_str = "ERROR";
    break;
  case LOG_EVENT:
    level_str = "EVENT";
    break;
  default:
    break;
  }

  fprintf(log_file, "[%s] [%s] [%s] ", time_str, component_name, level_str);

  va_list args;
  va_start(args, format);
  vfprintf(log_file, format, args);
  va_end(args);

  fprintf(log_file, "\n");
  fflush(log_file);
}

void log_close() {
  if (log_file) {
    log_message(LOG_INFO, "Logger closing");
    fclose(log_file);
    log_file = NULL;
  }
}
