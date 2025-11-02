\
#include "common.h"
#include <stdlib.h>
#include <string.h>

void queue_init(str_queue_t *q){
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->c, NULL);
    q->closed = 0;
}

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

void queue_close(str_queue_t *q){
    pthread_mutex_lock(&q->m);
    q->closed = 1;
    pthread_cond_broadcast(&q->c);
    pthread_mutex_unlock(&q->m);
}
