\
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

#define PORT 9000

// Simple node for queues
typedef struct str_node {
    char *line;
    struct str_node *next;
} str_node_t;

// Thread-safe queue for strings
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

#endif
