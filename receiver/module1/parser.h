/**
 * parser.h
 * 
 * Declarations for JSON parsing functions in module1.
 */

#ifndef MODULE1_PARSER_H
#define MODULE1_PARSER_H

#include "../types.h"

void parse_json_to_datapoint(const char *s, data_point_t *d);
int convert_json_to_datapoint(const char *s, data_point_t *d);

#endif
