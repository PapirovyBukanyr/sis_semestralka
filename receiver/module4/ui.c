/*
 * ui.c
 *
 * Minimal user interface / status reporting for the receiver application.
 */

#include <stdio.h>
#include <stdlib.h>

#include "../common.h"
#include "../queues.h"
#include "../log.h"

/**
 * UI thread function.
 *
 * Consumes messages from the `error_queue` and logs them using the project's
 * logging facility. The function returns when the queue is closed.
 *
 * @param arg unused thread argument
 * @return NULL
 */
void* ui_thread(void* arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&error_queue);
        if(!line) break;
        LOG_ERROR("[ui] error: %s\n", line);
        free(line);
    }
    return NULL;
}
