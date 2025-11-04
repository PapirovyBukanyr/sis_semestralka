/* Shared data types used across receiver modules */
#ifndef RECEIVER_TYPES_H
#define RECEIVER_TYPES_H

#include <math.h>

/* Data point structure matching JSON fields (as floats) */
typedef struct {
  double timestamp; /* original timestamp (seconds) */
  float export_bytes;
  float export_flows;
  float export_packets;
  float export_rtr;
  float export_rtt;
  float export_srt;
} data_point_t;

/* Minimal message structure used for printing received data */
typedef struct {
  long long ts;   /* timestamp, either parsed from message or receive time (ms) */
  char payload[8192]; /* raw payload (truncated if larger) */
  char src_addr[64];
  int src_port;
} recv_msg_t;

#endif /* RECEIVER_TYPES_H */
