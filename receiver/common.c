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
