\
#include "../common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../module2/nn.h"
#include "../module3/represent.h"

// We'll receive raw lines from a queue (provided via arg)
extern str_queue_t raw_queue;
extern str_queue_t proc_queue;
extern str_queue_t error_queue;

void* preproc_thread(void *arg){
    (void)arg;
    FILE *logf = fopen("receiver/module1/errors.log", "w");
    if(!logf) logf = stderr;
    while(1){
        char *line = queue_pop(&raw_queue);
        if(!line) break;
        // trim
        char *p = line;
        while(*p && isspace((unsigned char)*p)) p++;
        if(*p==0){ free(line); continue; }
        // parse: ts,bytes_sent,bytes_recv
        long long ts=0;
        int bs=-1, br=-1;
        int fields = sscanf(p, "%lld,%d,%d", &ts, &bs, &br);
        if(fields < 3){
            // malformed -> push to error log queue
            fprintf(logf, "Malformed or missing fields: %s\n", p);
            fflush(logf);
            queue_push(&error_queue, p);
            free(line);
            continue;
        }
        // detect negative values
        if(bs < 0 || br < 0){
            fprintf(logf, "Invalid negative value: %s\n", p);
            fflush(logf);
            queue_push(&error_queue, p);
            free(line);
            continue;
        }
        // convert to normalized representation (simple CSV)
        char out[256];
        snprintf(out, sizeof(out), "%lld,%d,%d", ts, bs, br);
        queue_push(&proc_queue, out);
        free(line);
    }
    if(logf != stderr) fclose(logf);
    return NULL;
}
