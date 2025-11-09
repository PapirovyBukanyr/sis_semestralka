/**
 * common.h
 * 
 * Common definitions for the receiver application, including thread-safe string queue structure and associated functions.
 */
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

#define PORT 9000

/**
 * Node in a string queue.
 * 
 * char *line: stored string
 * struct str_node *next: pointer to next node
 */
typedef struct str_node {
    char *line;
    struct str_node *next;
} str_node_t;

/**
 * Thread-safe string queue structure.
 * 
 * str_node_t *head: pointer to the head node
 * str_node_t *tail: pointer to the tail node
 * pthread_mutex_t m: mutex for synchronizing access
 * pthread_cond_t c: condition variable for signaling
 * int closed: flag indicating if the queue is closed
 */
typedef struct str_queue {
    str_node_t *head, *tail;
    pthread_mutex_t m;
    pthread_cond_t c;
    int closed;
} str_queue_t;

void queue_init(str_queue_t *q);
void queue_push(str_queue_t *q, const char *s);
char* queue_pop(str_queue_t *q); // caller must free
void queue_close(str_queue_t *q);

/* Non-blocking pop: returns a popped string or NULL immediately if the queue is empty.
 * Caller must free the returned pointer when non-NULL.
 */
char* queue_try_pop(str_queue_t *q);

/* Return the number of items currently queued. This iterates the list under
 * the queue mutex and may be O(n).
 */
int queue_length(str_queue_t *q);

void stats_init(void);
void stats_inc_received(void);
void stats_inc_processed(void);
void stats_inc_represented(void);
void stats_get_counts(long long *received, long long *processed, long long *represented);

#define STATS_WINDOW_SECONDS 60

void stats_record_prediction_error(double abs_err);

void stats_get_window_rates(int window_sec, long long *received, long long *processed, long long *represented);

void stats_get_avg_error(int window_sec, double *avg);

#endif
