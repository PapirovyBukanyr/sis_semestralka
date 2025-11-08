/* Thread-safe logger wrapper (cross-platform) */
#ifndef RECEIVER_LOG_H
#define RECEIVER_LOG_H

#include <stdio.h>

/* Initialize/destroy logger (setup mutex). Call once at program start/exit. */
void log_init(void);
void log_close(void);

/* Thread-safe fprintf wrapper. Keeps output from different threads from interleaving. */
void log_fprintf(FILE *f, const char *fmt, ...);

/* Convenience macros */
#define LOG_INFO(...)  log_fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) log_fprintf(stderr, __VA_ARGS__)

#endif /* RECEIVER_LOG_H */
