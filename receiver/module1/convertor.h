/* convertor.h - simple JSON -> C data conversion for module1 */
#ifndef CONVERTOR_H
#define CONVERTOR_H

#include <stddef.h>

/* Data point structure for numeric fields parsed from JSON */
typedef struct {
    double timestamp; /* seconds */
    float export_bytes;
    float export_flows;
    float export_packets;
    float export_rtr;
    float export_rtt;
    float export_srt;
} data_point_t;

/* Parse a flat JSON object (as a C string) and fill out 'out'.
 * Returns 1 if at least one numeric field was parsed, 0 otherwise.
 * This is a lightweight parser intended for the project's simple JSON lines.
 */
int convert_json_to_datapoint(const char *s, data_point_t *out);

#endif /* CONVERTOR_H */
