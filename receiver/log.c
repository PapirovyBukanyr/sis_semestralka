#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static CRITICAL_SECTION log_lock;
#else
#include <pthread.h>
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

void log_init(void){
#ifdef _WIN32
    InitializeCriticalSection(&log_lock);
#else
    pthread_mutex_init(&log_lock, NULL);
#endif
}

void log_close(void){
#ifdef _WIN32
    DeleteCriticalSection(&log_lock);
#else
    pthread_mutex_destroy(&log_lock);
#endif
}

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
