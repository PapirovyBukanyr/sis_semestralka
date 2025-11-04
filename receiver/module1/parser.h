/* parser.h - input parsing utilities (moved into module1 as it deals with input)
 */
#ifndef MODULE1_PARSER_H
#define MODULE1_PARSER_H

#include "../types.h"

/* Parse a simple flat JSON object with numeric fields into data_point_t */
void parse_json_to_datapoint(const char *s, data_point_t *d);

#endif /* MODULE1_PARSER_H */
