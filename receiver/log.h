/**
 * log.h
 * 
 * Declarations for thread-safe logging functions used across the receiver application.
 */
#ifndef RECEIVER_LOG_H
#define RECEIVER_LOG_H

#include <stdio.h>

void log_init(void);
void log_close(void);

void log_fprintf(FILE *f, const char *fmt, ...);

#define LOG_INFO(...)  log_fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) log_fprintf(stderr, __VA_ARGS__)

#endif 
