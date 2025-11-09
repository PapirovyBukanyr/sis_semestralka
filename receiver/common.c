/*
 * common.c
 *
 * Implementation of shared utility functions used throughout the receiver
 * pipeline (string helpers, small helpers used by modules).
 */
#ifndef COMMON_C_HEADER
#define COMMON_C_HEADER

#include "common.h"

#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/**
 * Initialize a string queue.
 *
 * Sets head/tail to NULL, initializes the mutex and condition variable and
 * clears the closed flag.
 *
 * @param q pointer to the queue to initialize
 */
void queue_init(str_queue_t *q){
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->c, NULL);
    q->closed = 0;
}

/**
 * Push a copy of the string onto the queue.
 *
 * The string is duplicated with strdup and owned by the queue; callers must
 * not free the provided pointer after calling this function.
 *
 * @param q target queue
 * @param s NUL-terminated C string to push
 */
void queue_push(str_queue_t *q, const char *s){
    str_node_t *n = malloc(sizeof(*n));
    n->next = NULL;
    n->line = strdup(s);
    pthread_mutex_lock(&q->m);
    if(q->tail) q->tail->next = n; else q->head = n;
    q->tail = n;
    pthread_cond_signal(&q->c);
    pthread_mutex_unlock(&q->m);
}

/**
 * Pop a string from the queue.
 *
 * This function blocks until an item is available or the queue is closed.
 * The returned pointer is owned by the caller and must be freed by the
 * caller when no longer needed. Returns NULL when the queue is closed and
 * empty.
 *
 * @param q source queue
 * @return allocated string pointer or NULL
 */
char* queue_pop(str_queue_t *q){
    pthread_mutex_lock(&q->m);
    while(!q->head && !q->closed) pthread_cond_wait(&q->c, &q->m);
    if(!q->head){
        pthread_mutex_unlock(&q->m);
        return NULL;
    }
    str_node_t *n = q->head;
    q->head = n->next;
    if(!q->head) q->tail = NULL;
    pthread_mutex_unlock(&q->m);
    char *s = n->line;
    free(n);
    return s;
}

/**
 * Close the queue and wake any waiting consumers.
 *
 * After calling this, `queue_pop` will return NULL once the queue is empty.
 *
 * @param q queue to close
 */
void queue_close(str_queue_t *q){
    pthread_mutex_lock(&q->m);
    q->closed = 1;
    pthread_cond_broadcast(&q->c);
    pthread_mutex_unlock(&q->m);
}

char* queue_try_pop(str_queue_t *q){
    pthread_mutex_lock(&q->m);
    if(!q->head){
        pthread_mutex_unlock(&q->m);
        return NULL;
    }
    str_node_t *n = q->head;
    q->head = n->next;
    if(!q->head) q->tail = NULL;
    pthread_mutex_unlock(&q->m);
    char *s = n->line;
    free(n);
    return s;
}

/**
 * Return the number of items currently queued. This iterates the list under
 * the queue mutex and may be O(n).
 *
 * @param q queue to inspect
 * @return number of items currently in the queue
 */
int queue_length(str_queue_t *q){
    int cnt = 0;
    pthread_mutex_lock(&q->m);
    for(str_node_t *n = q->head; n; n = n->next) ++cnt;
    pthread_mutex_unlock(&q->m);
    return cnt;
}

static pthread_mutex_t stats_m = PTHREAD_MUTEX_INITIALIZER;
static long long stats_received = 0;
static long long stats_processed = 0;
static long long stats_represented = 0;

/* Moving-window buckets (one-second resolution) */
#define STATS_WINDOW_SECONDS 60
static time_t bucket_ts[STATS_WINDOW_SECONDS];
static long long bucket_recv[STATS_WINDOW_SECONDS];
static long long bucket_proc[STATS_WINDOW_SECONDS];
static long long bucket_repr[STATS_WINDOW_SECONDS];

/* Simple ring buffer for prediction-error samples */
#define ERR_RING_SIZE 1024
static time_t err_ts[ERR_RING_SIZE];
static double err_val[ERR_RING_SIZE];
static int err_head = 0;

static void stats_add_to_bucket(long long *buckets, time_t now, long long delta){
    int idx = (int)(now % STATS_WINDOW_SECONDS);
    if(bucket_ts[idx] != now){
        bucket_ts[idx] = now;
        buckets[idx] = 0;
    }
    buckets[idx] += delta;
}

/**
 * Initialize statistics counters to zero.
 */
void stats_init(void){
    pthread_mutex_lock(&stats_m);
    stats_received = stats_processed = stats_represented = 0;
    for(int i=0;i<STATS_WINDOW_SECONDS;i++){ bucket_ts[i] = 0; bucket_recv[i]=bucket_proc[i]=bucket_repr[i]=0; }
    for(int i=0;i<ERR_RING_SIZE;i++){ err_ts[i]=0; err_val[i]=0.0; }
    err_head = 0;
    pthread_mutex_unlock(&stats_m);
}

/**
 * Increment the received messages counter.
 */
void stats_inc_received(void){
    pthread_mutex_lock(&stats_m);
    ++stats_received;
    stats_add_to_bucket(bucket_recv, time(NULL), 1);
    pthread_mutex_unlock(&stats_m);
}

/**
 * Increment the processed messages counter.
 */
void stats_inc_processed(void){
    pthread_mutex_lock(&stats_m);
    ++stats_processed;
    stats_add_to_bucket(bucket_proc, time(NULL), 1);
    pthread_mutex_unlock(&stats_m);
}

/**
 * Increment the represented messages counter.
 */
void stats_inc_represented(void){
    pthread_mutex_lock(&stats_m);
    ++stats_represented;
    stats_add_to_bucket(bucket_repr, time(NULL), 1);
    pthread_mutex_unlock(&stats_m);
}

/**
 * Record a prediction error sample.
 *
 * @param abs_err absolute error value to record
 */
void stats_record_prediction_error(double abs_err){
    pthread_mutex_lock(&stats_m);
    time_t now = time(NULL);
    err_ts[err_head] = now;
    err_val[err_head] = abs_err;
    err_head = (err_head + 1) % ERR_RING_SIZE;
    pthread_mutex_unlock(&stats_m);
}

/**
 * Get per-second rates over the specified window.
 *
 * @param window_sec window size in seconds (max STATS_WINDOW_SECONDS)
 * @param received pointer receiving received rate or NULL
 * @param processed pointer receiving processed rate or NULL
 * @param represented pointer receiving represented rate or NULL
 */
void stats_get_window_rates(int window_sec, long long *received, long long *processed, long long *represented){
    if(window_sec <= 0) window_sec = 1;
    if(window_sec > STATS_WINDOW_SECONDS) window_sec = STATS_WINDOW_SECONDS;
    time_t now = time(NULL);
    pthread_mutex_lock(&stats_m);
    long long r=0,p=0,rep=0;
    for(int i=0;i<STATS_WINDOW_SECONDS;i++){
        if(now - bucket_ts[i] < window_sec){ r += bucket_recv[i]; p += bucket_proc[i]; rep += bucket_repr[i]; }
    }
    if(received) *received = r;
    if(processed) *processed = p;
    if(represented) *represented = rep;
    pthread_mutex_unlock(&stats_m);
}

/**
 * Get average prediction error over the specified window.
 *
 * @param window_sec window size in seconds (max STATS_WINDOW_SECONDS)
 * @param avg pointer receiving the average error or NAN if no samples
 */
void stats_get_avg_error(int window_sec, double *avg){
    if(window_sec <= 0) window_sec = 1;
    if(window_sec > STATS_WINDOW_SECONDS) window_sec = STATS_WINDOW_SECONDS;
    time_t now = time(NULL);
    pthread_mutex_lock(&stats_m);
    double sum = 0.0; int cnt = 0;
    for(int i=0;i<ERR_RING_SIZE;i++){
        if(err_ts[i] == 0) continue;
        if(now - err_ts[i] < window_sec){ sum += err_val[i]; ++cnt; }
    }
    if(avg){ if(cnt==0) *avg = NAN; else *avg = sum / (double)cnt; }
    pthread_mutex_unlock(&stats_m);
}

/**
 * Get the current statistics counters.
 *
 * @param received pointer receiving the received messages count or NULL
 * @param processed pointer receiving the processed messages count or NULL
 * @param represented pointer receiving the represented messages count or NULL
 */
void stats_get_counts(long long *received, long long *processed, long long *represented){
    pthread_mutex_lock(&stats_m);
    if(received) *received = stats_received;
    if(processed) *processed = stats_processed;
    if(represented) *represented = stats_represented;
    pthread_mutex_unlock(&stats_m);
}
