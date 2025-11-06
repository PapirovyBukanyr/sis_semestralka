#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <stdio.h>
#include "../types.h"

// Function declarations
void process_input(const char *input_file);
void parse_data(const char *raw_data);
void convert_data(const char *parsed_data);
/* Expose parser helper so other modules (e.g. io.c) can call it */
void parse_json_to_datapoint(const char *s, data_point_t *d);
int convert_json_to_datapoint(const char *s, data_point_t *d);

#endif // DATA_PROCESSOR_H