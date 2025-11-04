/* convertor.h - simple JSON -> C data conversion for module1 */
#ifndef CONVERTOR_H
#define CONVERTOR_H

#include <stddef.h>
#include "../types.h"

/* Parse a flat JSON object (as a C string) and fill out 'out'.
 * Returns 1 if at least one numeric field was parsed, 0 otherwise.
 * This is a lightweight parser intended for the project's simple JSON lines.
 */
int convert_json_to_datapoint(const char *s, data_point_t *out);

#endif /* CONVERTOR_H */
