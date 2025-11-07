#include <stdio.h>
#include <stdlib.h>
#include "../common.h"
#include "../queues.h"

void* represent_thread(void* arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&repr_queue);
        if(!line) break;
        printf("[represent] %s\n", line);
        fflush(stdout);
        free(line);
    }
    return NULL;
}
