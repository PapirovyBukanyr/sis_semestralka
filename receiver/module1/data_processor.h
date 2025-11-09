/** 
 * data_processor.h
 * 
 * Declarations for data processing functions in module1.
 */

#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <stdio.h>
#include "../types.h"


void process_input(const char *input_file);
void parse_data(const char *raw_data);
void convert_data(const char *parsed_data);
void parse_json_to_datapoint(const char *s, data_point_t *d);
int convert_json_to_datapoint(const char *s, data_point_t *d);

#endif 