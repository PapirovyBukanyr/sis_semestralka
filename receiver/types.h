/**
 * types.h
 * 
 * Common type definitions used across the receiver application and its modules.
 */

#ifndef RECEIVER_TYPES_H
#define RECEIVER_TYPES_H

#include <math.h>

/**
 * Data point structure representing a single measurement.
 * 
 * double timestamp: original timestamp in seconds
 * float export_bytes: exported bytes
 * float export_flows: exported flows
 * float export_packets: exported packets 
 * float export_rtr: exported router reports
 * float export_rtt: exported round-trip time
 * float export_srt: exported smoothed round-trip time
 */
typedef struct {
  double timestamp;
  float export_bytes;
  float export_flows;
  float export_packets;
  float export_rtr;
  float export_rtt;
  float export_srt;
} data_point_t;

/**
 * Received message structure representing a message received by the receiver.
 *
 * ts: timestamp in milliseconds
 * payload: raw message payload
 * src_addr: source IP address as string
 * src_port: source port number
 */
typedef struct {
  long long ts;
  char payload[8192]; 
  char src_addr[64];
  int src_port;
} recv_msg_t;

#endif
