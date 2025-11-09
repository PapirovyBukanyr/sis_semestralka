/*
 * parser.c
 *
 * Lightweight JSON/CSV parsing helpers used by the data processing module.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef PARSER_C_HEADER
#define PARSER_C_HEADER
#include "parser.h"
#endif

/**
 * Locate a numeric value after a JSON key name.
 *
 * Searches for `key` in `s`, finds the following ':' and attempts to parse
 * a floating-point number. This helper is intentionally permissive and used
 * for lightweight JSON-like extraction in the receiver pipeline.
 *
 * @param s input string to search
 * @param key key name to find (e.g. "\"timestamp\"")
 * @param out pointer receiving the parsed double when found
 * @return 1 if a numeric value was found and parsed, 0 otherwise
 */
static int find_json_number(const char *s, const char *key, double *out) {
  const char *p = strstr(s, key);
  if(!p) return 0;
  /* find ':' after key */
  const char *c = strchr(p, ':');
  if(!c) return 0;
  /* skip whitespace */
  c++;
  while(*c && (*c==' ' || *c=='\t')) c++;
  char *endptr;
  double v = strtod(c, &endptr);
  if(c == endptr) return 0;
  *out = v; return 1;
}

/**
 * Parse a lightweight JSON-like string into a data_point_t.
 *
 * Fields that cannot be found are left as NaN so callers can detect missing
 * values. Supported keys: timestamp, export_bytes, export_flows,
 * export_packets, export_rtr, export_rtt, export_srt. Matching accepts keys
 * with or without surrounding quotes.
 *
 * @param s input NUL-terminated string containing JSON-like content
 * @param d output pointer to data_point_t (will be written with parsed values or NaN)
 */
void parse_json_to_datapoint(const char *s, data_point_t *d){
  d->timestamp = NAN;
  d->export_bytes = NAN;
  d->export_flows = NAN;
  d->export_packets = NAN;
  d->export_rtr = NAN;
  d->export_rtt = NAN;
  d->export_srt = NAN;
  double tmp;
  if(find_json_number(s, "\"timestamp\"", &tmp) || find_json_number(s, "timestamp", &tmp)) d->timestamp = tmp;
  if(find_json_number(s, "\"export_bytes\"", &tmp) || find_json_number(s, "export_bytes", &tmp)) d->export_bytes = (float)tmp;
  if(find_json_number(s, "\"export_flows\"", &tmp) || find_json_number(s, "export_flows", &tmp)) d->export_flows = (float)tmp;
  if(find_json_number(s, "\"export_packets\"", &tmp) || find_json_number(s, "export_packets", &tmp)) d->export_packets = (float)tmp;
  if(find_json_number(s, "\"export_rtr\"", &tmp) || find_json_number(s, "export_rtr", &tmp)) d->export_rtr = (float)tmp;
  if(find_json_number(s, "\"export_rtt\"", &tmp) || find_json_number(s, "export_rtt", &tmp)) d->export_rtt = (float)tmp;
  if(find_json_number(s, "\"export_srt\"", &tmp) || find_json_number(s, "export_srt", &tmp)) d->export_srt = (float)tmp;
}
