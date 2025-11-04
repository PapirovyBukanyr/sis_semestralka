#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Helper: find a numeric value after a JSON key name; returns 1 on found */
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

void parse_json_to_datapoint(const char *s, data_point_t *d){
  /* initialize to NaN */
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
