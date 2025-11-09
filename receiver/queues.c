/*
 * queues.c
 *
 * Definitions for the shared queue instances used for inter-thread
 * communication in the receiver pipeline.
 */

#ifndef QUEUES_C_HEADER
#define QUEUES_C_HEADER
#include "queues.h"
#endif

#include "common.h"

str_queue_t raw_queue;
str_queue_t proc_queue;
str_queue_t repr_queue;
str_queue_t error_queue;
