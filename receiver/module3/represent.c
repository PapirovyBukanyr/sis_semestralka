#include <stdio.h>
#include <stdlib.h>
#include "../common.h"
#include "../queues.h"
#include "../log.h"

void* represent_thread(void* arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&repr_queue);
        if(!line) break;
    LOG_INFO("[represent] %s\n", line);
        free(line);
    }
    return NULL;
}
