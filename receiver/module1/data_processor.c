/*
 * data_processor.c
 *
 * Data cleaning and feature extraction used to prepare rows for the
 * neural-network pipeline.
 */

#ifndef DATA_PROCESSOR_C_HEADER
#define DATA_PROCESSOR_C_HEADER

#include "data_processor.h"

#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "parser.h"
#include "../common.h"
#include "../queues.h"

/**
 * Process an input file line-by-line.
 *
 * Example utility that reads `input_file` and calls `parse_data` for each
 * line. This helper is used during local testing and is not required for the
 * network receiver pipeline.
 *
 * @param input_file path to an input text file
 */
void process_input(const char *input_file) {
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

/**
 * Parse a raw input string.
 *
 * This example function demonstrates how raw input could be parsed. It
 * currently forwards the parsed content to `convert_data` for conversion.
 *
 * @param raw_data input string (NUL-terminated)
 */
void parse_data(const char *raw_data) {
    printf("Parsing data: %s\n", raw_data);
    convert_data(raw_data);
}

/**
 * Convert parsed data into the target representation.
 *
 * Placeholder used by the example parsing pipeline.
 *
 * @param parsed_data parsed input string
 */
void convert_data(const char *parsed_data) {
    printf("Converting data: %s\n", parsed_data);
}

/**
 * Convert a JSON-like string to a data_point_t using the parser helpers.
 *
 * Returns 1 when at least one numeric field was present, 0 otherwise.
 *
 * @param s input JSON-like string
 * @param d output datapoint
 * @return 1 on success (some numeric data found), 0 otherwise
 */
int convert_json_to_datapoint(const char *s, data_point_t *d){
  parse_json_to_datapoint(s, d);
  if(!isnan(d->timestamp)) return 1;
  if(!isnan((double)d->export_bytes)) return 1;
  if(!isnan((double)d->export_flows)) return 1;
  if(!isnan((double)d->export_packets)) return 1;
  if(!isnan((double)d->export_rtr)) return 1;
  if(!isnan((double)d->export_rtt)) return 1;
  if(!isnan((double)d->export_srt)) return 1;
  return 0;
}

/**
 * Preprocessing thread for the pipeline.
 *
 * Reads raw lines from `raw_queue`, attempts to parse them into
 * `data_point_t` and forwards either a CSV-formatted string expected by the
 * NN thread or the original raw line to `proc_queue`.
 *
 * @param arg unused thread argument
 * @return NULL
 */
void *preproc_thread(void *arg){
    (void)arg;
    while(1){
        char *line = queue_pop(&raw_queue);
        if(!line) break;

        data_point_t dp;
        if(convert_json_to_datapoint(line, &dp)){
            char outbuf[512];
            long long ts_ms = 0;
            if(!isnan(dp.timestamp)) ts_ms = (long long)(dp.timestamp * 1000.0);
            /* We don't need the formatted-length return value here, avoid unused-variable warning */
            snprintf(outbuf, sizeof(outbuf), "%lld,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
                     ts_ms,
                     (double)dp.export_bytes,
                     (double)dp.export_flows,
                     (double)dp.export_packets,
                     (double)dp.export_rtr,
                     (double)dp.export_rtt,
                     (double)dp.export_srt);
            char *p = strdup(outbuf);
            if(p) {
                queue_push(&proc_queue, p);
                stats_inc_processed();
            }
        } else {
            queue_push(&proc_queue, line);
            stats_inc_processed();
            free(line);
            continue;
        }
        free(line);
    }
    return NULL;
}