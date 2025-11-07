#include <stdio.h>
#include <stdlib.h>
#include "../common.h"
#include "../queues.h"

void* ui_thread(void* arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&error_queue);
        if(!line) break;
        fprintf(stderr, "[ui] error: %s\n", line);
        free(line);
    }
    return NULL;
}
