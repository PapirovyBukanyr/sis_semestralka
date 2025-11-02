\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../module2/nn.h"

extern str_queue_t proc_queue;
extern str_queue_t repr_queue;
extern str_queue_t error_queue;

// Simple rule-based mapping from prediction to recommended action.
static const char* recommend_for(double score){
    if(score > 0.9) return "IMMEDIATE THROTTLE";
    if(score > 0.75) return "Reduce non-critical flows";
    if(score > 0.5) return "Monitor closely";
    return "Normal";
}

void* represent_thread(void *arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&proc_queue);
        if(!line) break;
        // We expect lines of form: ts,pred,acc
        long long ts;
        double pred, acc;
        if(sscanf(line, "%lld,%lf,%lf", &ts, &pred, &acc) == 3){
            // create representation: timestamp,score,recommendation,accuracy
            char out[256];
            const char *rec = recommend_for(pred);
            snprintf(out, sizeof(out), "%lld,%.6f,%s,%.6f", ts, pred, rec, acc);
            queue_push(&repr_queue, out);
        } else {
            // if not matching, forward to error log
            queue_push(&error_queue, line);
        }
        free(line);
    }
    return NULL;
}
