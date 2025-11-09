/*
 * log.c
 *
 * Logging helpers and platform-adapted output routines used by receiver
 * components.
 */

#ifndef LOG_C_HEADER
#define LOG_C_HEADER

#include "log.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "platform.h"

#ifdef _WIN32
#include <windows.h>
static CRITICAL_SECTION log_lock;
#else
#include <pthread.h>
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/**
 * Initialize logging subsystem resources.
 */
void log_init(void){
#ifdef _WIN32
    InitializeCriticalSection(&log_lock);
#else
    pthread_mutex_init(&log_lock, NULL);
#endif
}

/**
 * Release logging subsystem resources.
 */
void log_close(void){
#ifdef _WIN32
    DeleteCriticalSection(&log_lock);
#else
    pthread_mutex_destroy(&log_lock);
#endif
}

/**
 * Thread-safe fprintf wrapper used by the project for serialized logging.
 *
 * This function acquires the internal log lock, performs a vfprintf to the
 * provided FILE* and flushes the stream. If `f` is NULL the call is a no-op.
 *
 * @param f FILE pointer to write to (e.g., stdout/stderr or a log file)
 * @param fmt printf-style format string
 * @param ... format arguments
 */
void log_fprintf(FILE *f, const char *fmt, ...){
    if(!f) return;
#ifdef _WIN32
    EnterCriticalSection(&log_lock);
#else
    pthread_mutex_lock(&log_lock);
#endif
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fflush(f);
#ifdef _WIN32
    LeaveCriticalSection(&log_lock);
#else
    pthread_mutex_unlock(&log_lock);
#endif
}
