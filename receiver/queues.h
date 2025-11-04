/* Central declaration of pipeline queues used across modules */
#ifndef RECEIVER_QUEUES_H
#define RECEIVER_QUEUES_H

#include "common.h"

extern str_queue_t raw_queue;
extern str_queue_t proc_queue;
extern str_queue_t repr_queue;
extern str_queue_t error_queue;

#endif /* RECEIVER_QUEUES_H */
