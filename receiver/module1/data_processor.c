#include "data_processor.h"
#include "parser.h"
#include "../common.h"
#include "../queues.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Function to process input
void process_input(const char *input_file) {
    // Example: Read file, parse, and convert
    FILE *file = fopen(input_file, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        parse_data(buffer);
    }

    fclose(file);
}

// Function to parse raw data
void parse_data(const char *raw_data) {
    // Example parsing logic
    printf("Parsing data: %s\n", raw_data);
    // Call conversion after parsing
    convert_data(raw_data);
}

// Function to convert parsed data
void convert_data(const char *parsed_data) {
    // Example conversion logic
    printf("Converting data: %s\n", parsed_data);
}

/* parser.c implements parse_json_to_datapoint and helpers; we call it here. */

int convert_json_to_datapoint(const char *s, data_point_t *d){
  parse_json_to_datapoint(s, d);
  /* Consider success if any numeric field was found */
  if(!isnan(d->timestamp)) return 1;
  if(!isnan((double)d->export_bytes)) return 1;
  if(!isnan((double)d->export_flows)) return 1;
  if(!isnan((double)d->export_packets)) return 1;
  if(!isnan((double)d->export_rtr)) return 1;
  if(!isnan((double)d->export_rtt)) return 1;
  if(!isnan((double)d->export_srt)) return 1;
  return 0;
}

/* Minimal preproc thread: consume raw_queue and forward lines to proc_queue.
     This is a simple placeholder so the pipeline threads can be started and
     messages flow to the NN thread. More advanced parsing/persistence can be
     implemented here later. */
void *preproc_thread(void *arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&raw_queue);
        if(!line) break; /* queue closed */

        /* Parse JSON into data_point_t; if parsing fails, forward raw line for compatibility */
        data_point_t dp;
        if(convert_json_to_datapoint(line, &dp)){
            /* Format as CSV expected by nn_thread: ts,export_bytes,export_flows,export_packets,export_rtr,export_rtt,export_srt
               Use timestamp as integer ms if available, otherwise 0. */
            char outbuf[512];
            long long ts_ms = 0;
            if(!isnan(dp.timestamp)) ts_ms = (long long)(dp.timestamp * 1000.0);
            int off = snprintf(outbuf, sizeof(outbuf), "%lld,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
                               ts_ms,
                               (double)dp.export_bytes,
                               (double)dp.export_flows,
                               (double)dp.export_packets,
                               (double)dp.export_rtr,
                               (double)dp.export_rtt,
                               (double)dp.export_srt);
            /* push duplicated string onto proc_queue */
            char *p = strdup(outbuf);
            if(p) queue_push(&proc_queue, p);
        } else {
            /* fallback: forward raw line unchanged */
            queue_push(&proc_queue, line);
            free(line);
            continue;
        }
        free(line);
    }
    return NULL;
}