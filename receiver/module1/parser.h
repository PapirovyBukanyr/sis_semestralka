#ifndef MODULE1_PARSER_H
#define MODULE1_PARSER_H

#include "../types.h"

/* Parse JSON-like string into data_point_t. Always fills the struct (fields set to NaN if not present). */
void parse_json_to_datapoint(const char *s, data_point_t *d);

/* Convert JSON-like string into data_point_t. Returns 1 on success (at least one numeric field found), 0 on failure. */
int convert_json_to_datapoint(const char *s, data_point_t *d);

#endif /* MODULE1_PARSER_H */
