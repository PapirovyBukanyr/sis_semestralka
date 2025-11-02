/* Lightweight JSON numeric extractor for module1
 * This file implements convert_json_to_datapoint declared in convertor.h
 */
#include "convertor.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Helper: find a numeric value after a JSON key name; returns 1 on found */
static int find_json_number(const char *s, const char *key, double *out) {
    const char *p = strstr(s, key);
    if (!p) return 0;
    const char *c = strchr(p, ':');
    if (!c) return 0;
    c++;
    while (*c && (*c == ' ' || *c == '\t')) c++;
    char *endptr;
    double v = strtod(c, &endptr);
    if (c == endptr) return 0;
    *out = v; return 1;
}

int convert_json_to_datapoint(const char *s, data_point_t *out) {
    if (!s || !out) return 0;
    /* initialize to NaN */
    out->timestamp = NAN;
    out->export_bytes = NAN;
    out->export_flows = NAN;
    out->export_packets = NAN;
    out->export_rtr = NAN;
    out->export_rtt = NAN;
    out->export_srt = NAN;

    double tmp;
    int any = 0;
    if (find_json_number(s, "\"timestamp\"", &tmp) || find_json_number(s, "timestamp", &tmp)) { out->timestamp = tmp; any = 1; }
    if (find_json_number(s, "\"export_bytes\"", &tmp) || find_json_number(s, "export_bytes", &tmp)) { out->export_bytes = (float)tmp; any = 1; }
    if (find_json_number(s, "\"export_flows\"", &tmp) || find_json_number(s, "export_flows", &tmp)) { out->export_flows = (float)tmp; any = 1; }
    if (find_json_number(s, "\"export_packets\"", &tmp) || find_json_number(s, "export_packets", &tmp)) { out->export_packets = (float)tmp; any = 1; }
    if (find_json_number(s, "\"export_rtr\"", &tmp) || find_json_number(s, "export_rtr", &tmp)) { out->export_rtr = (float)tmp; any = 1; }
    if (find_json_number(s, "\"export_rtt\"", &tmp) || find_json_number(s, "export_rtt", &tmp)) { out->export_rtt = (float)tmp; any = 1; }
    if (find_json_number(s, "\"export_srt\"", &tmp) || find_json_number(s, "export_srt", &tmp)) { out->export_srt = (float)tmp; any = 1; }

    return any;
}
