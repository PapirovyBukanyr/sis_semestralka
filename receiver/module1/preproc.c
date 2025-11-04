\
#include "../common.h"
#include "convertor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../module2/nn.h"
#include "../module3/represent.h"
#include <stdint.h>
#include <math.h>

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
        // If the incoming line looks like JSON (has a '{' or the export_bytes key),
        // parse it into a C data structure and forward a normalized CSV line to
        // the next queue. Otherwise fall back to the old CSV parsing logic.
        if(strchr(p, '{') != NULL || strstr(p, "export_bytes") != NULL){
            data_point_t dp;
            int ok = convert_json_to_datapoint(p, &dp);
            if(!ok){
                fprintf(logf, "JSON parse failed: %s\n", p);
                fflush(logf);
                queue_push(&error_queue, p);
                free(line);
                continue;
            }
            /* produce a normalized CSV line: timestamp,export_bytes,export_flows,export_packets,export_rtr,export_rtt,export_srt */
            char out[256];
            long long ts = (long long)dp.timestamp;
            snprintf(out, sizeof(out), "%lld,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
                ts,
                dp.export_bytes, dp.export_flows, dp.export_packets,
                dp.export_rtr, dp.export_rtt, dp.export_srt);
            /* persist history entry (ts + normalized inputs) to binary history file used by NN */
            {
                /* normalization must match NN normalization: in0 = export_bytes/1e6, in1 = export_flows/100 */
                typedef struct { int64_t ts; float in0; float in1; } history_entry_t;
                history_entry_t he;
                he.ts = ts;
                float in0 = (float)(dp.export_bytes / 1000000.0);
                if(!isfinite(in0) || in0 < 0.0f) in0 = 0.0f;
                if(in0 > 1.0f) in0 = 1.0f;
                float in1 = (float)(dp.export_flows / 100.0);
                if(!isfinite(in1) || in1 < 0.0f) in1 = 0.0f;
                if(in1 > 1.0f) in1 = 1.0f;
                he.in0 = in0; he.in1 = in1;
                FILE *hf = fopen("data/log_history.bin", "ab");
                if(hf){ fwrite(&he, sizeof(he), 1, hf); fclose(hf); }
            }
            queue_push(&proc_queue, out);
            free(line);
            continue;
        }
        /* parse: ts,bytes_sent,bytes_recv */
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
        /* persist history entry for legacy CSV inputs using legacy normalization */
        {
            typedef struct { int64_t ts; float in0; float in1; } history_entry_t;
            history_entry_t he;
            he.ts = ts;
            float in0 = (float)bs / 2000.0f; if(!isfinite(in0) || in0 < 0.0f) in0 = 0.0f; 
            if(in0 > 1.0f) in0 = 1.0f;
            float in1 = (float)br / 2000.0f; if(!isfinite(in1) || in1 < 0.0f) in1 = 0.0f; 
            if(in1 > 1.0f) in1 = 1.0f;
            he.in0 = in0; he.in1 = in1;
            FILE *hf = fopen("data/log_history.bin", "ab"); if(hf){ fwrite(&he, sizeof(he), 1, hf); fclose(hf); }
        }
        queue_push(&proc_queue, out);
        free(line);
    }
    if(logf != stderr) fclose(logf);
    return NULL;
}
