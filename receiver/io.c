/*
 * io.c
 *
 * I/O helpers and higher-level orchestration for receiving and processing
 * incoming data. Implements functions used by the receiver main loop.
 */

#ifndef IO_C_HEADER
#define IO_C_HEADER
#include "io.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "platform.h"
#include "types.h"
#include "common.h"
#include "queues.h"
#include "module1/data_processor.h"
#include "module2/nn.h"
#include "module3/represent.h"
#include "module4/ui.h"
#include "log.h"

/**
 * Safe copy helper to avoid -Wstringop-truncation on strncpy and ensure NUL termination.
 * 
 * @param dst destination buffer
 * @param src source string
 * @param dst_size size of destination buffer
 */
static void safe_strncpy(char *dst, const char *src, size_t dst_size){
  if(dst_size == 0) return;
  size_t len = strlen(src);
  if(len > dst_size - 1) len = dst_size - 1;
  memcpy(dst, src, len);
  dst[len] = '\0';
}

/**
 * Forward declaration for module1's preprocessing thread function expected by pthread_create. The implementation lives in module1 (data_processor.c / parser.c).
 */
void *preproc_thread(void *arg);

/**
 * Run the main receiver loop: initialize sockets, start pipeline threads, receive UDP messages and push them into the processing pipeline.
 */
int run_receiver(void){
  if (platform_socket_init() != 0) {
    return EXIT_FAILURE;
  }
  log_init();
  socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock == (socket_t)-1 || sock == INVALID_SOCKET){ perror("socket"); platform_socket_cleanup(); return 1; }
  struct sockaddr_in me;
  memset(&me,0,sizeof(me));
  me.sin_family = AF_INET;
  me.sin_port = htons(PORT);
  me.sin_addr.s_addr = INADDR_ANY;
  if(bind(sock, (struct sockaddr*)&me, sizeof(me))<0){ perror("bind"); CLOSESOCKET(sock); platform_socket_cleanup(); return 1; }
  queue_init(&raw_queue);
  queue_init(&proc_queue);
  queue_init(&repr_queue);
  queue_init(&error_queue);
  pthread_t t_preproc, t_nn, t_repr, t_ui;
  if(pthread_create(&t_preproc, NULL, preproc_thread, NULL) != 0){ perror("pthread_create preproc"); }
  if(pthread_create(&t_nn, NULL, nn_thread, NULL) != 0){ perror("pthread_create nn"); }
  if(pthread_create(&t_repr, NULL, represent_thread, NULL) != 0){ perror("pthread_create represent"); }
  if(pthread_create(&t_ui, NULL, ui_thread, NULL) != 0){ perror("pthread_create ui"); }
  LOG_INFO("Simple receiver listening on UDP port %d (pipeline threads started)\n", PORT);
  while(1){
    char buf[8192];
    struct sockaddr_in from; socklen_t flen = sizeof(from);
    int n = (int)recvfrom(sock, buf, (int)sizeof(buf)-1, 0, (struct sockaddr*)&from, &flen);
    if(n <= 0) continue;
    if(n >= (int)sizeof(buf)) n = (int)sizeof(buf)-1;
    buf[n] = '\0';
    queue_push(&raw_queue, buf);
    recv_msg_t m;
    memset(&m, 0, sizeof(m));
    if(buf[0] == '{'){
      safe_strncpy(m.payload, buf, sizeof(m.payload));
      m.ts = 0;
    } else {
      char *comma = strchr(buf, ',');
      int parsed_ts = 0;
      if(comma){
        const char *p = buf; while(*p==' ' || *p=='\t') p++;
        const char *q = comma - 1; while(q>p && (*q==' ' || *q=='\t')) q--;
        int has_digit = 0; const char *r = p;
        if(*r == '-' ) r++;
        for(; r<=q; ++r) if(*r >= '0' && *r <= '9') has_digit = 1; else if(*r == ' ' || *r == '\t') continue; else if(*r == '\0') break; else { has_digit = 0; break; }
        if(has_digit){
          char tbuf[64]; size_t tlen = (size_t)(comma - buf); if(tlen >= sizeof(tbuf)) tlen = sizeof(tbuf)-1;
          memcpy(tbuf, buf, tlen); tbuf[tlen]=0;
          long long v = atoll(tbuf);
          if(v != 0){ m.ts = v; parsed_ts = 1; }
          else { m.ts = 0; }
          safe_strncpy(m.payload, comma+1, sizeof(m.payload));
        }
      }
      if(!parsed_ts){
  safe_strncpy(m.payload, buf, sizeof(m.payload));
#ifdef _WIN32
        SYSTEMTIME st; FILETIME ft; GetSystemTime(&st); SystemTimeToFileTime(&st, &ft);
        unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        const unsigned long long EPOCH_DIFF = 116444736000000000ULL;
        m.ts = (long long)((t - EPOCH_DIFF) / 10000ULL);
#else
        struct timeval tv; gettimeofday(&tv, NULL); m.ts = (long long)tv.tv_sec; /* seconds */
#endif
      }
    }
    inet_ntop(AF_INET, &from.sin_addr, m.src_addr, sizeof(m.src_addr));
    m.src_port = ntohs(from.sin_port);
  LOG_INFO("--- received from %s:%d ---\n", m.src_addr, m.src_port);
    data_point_t dp; parse_json_to_datapoint(m.payload, &dp);
    int has_timestamp = !isnan(dp.timestamp);
    int has_export_bytes = !isnan(dp.export_bytes);
    if(has_timestamp || has_export_bytes || !isnan(dp.export_flows) || !isnan(dp.export_packets) || !isnan(dp.export_rtr) || !isnan(dp.export_rtt) || !isnan(dp.export_srt)){
  if(has_timestamp) m.ts = (long long)dp.timestamp;
  LOG_INFO("timestamp: %lld\n", m.ts);
  LOG_INFO("export_bytes: %.6f\n", dp.export_bytes);
  LOG_INFO("export_flows: %.6f\n", dp.export_flows);
  LOG_INFO("export_packets: %.6f\n", dp.export_packets);
  LOG_INFO("export_rtr: %.6f\n", dp.export_rtr);
  LOG_INFO("export_rtt: %.6f\n", dp.export_rtt);
  LOG_INFO("export_srt: %.6f\n", dp.export_srt);
    } else {
      LOG_INFO("timestamp: %lld\n", m.ts);
      LOG_INFO("payload: %s\n", m.payload);
    }
  }
  CLOSESOCKET(sock);
  platform_socket_cleanup();
  log_close();
  return EXIT_SUCCESS;
}
