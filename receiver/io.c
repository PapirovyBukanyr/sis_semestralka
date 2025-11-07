#include "platform.h"
#include "types.h"
#include "common.h"
#include "queues.h"
#include "io.h"
#include "module1/data_processor.h"
#include "module2/nn.h"
#include "module3/represent.h"
#include "module4/ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Forward declaration for module1's preprocessing thread function expected by
  pthread_create. The implementation lives in module1 (data_processor.c / parser.c). */
void *preproc_thread(void *arg);

int run_receiver(void){
  if (platform_socket_init() != 0) {
    return EXIT_FAILURE;
  }

  /* Create UDP socket and bind to PORT on all interfaces */
  socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock == (socket_t)-1 || sock == INVALID_SOCKET){ perror("socket"); platform_socket_cleanup(); return 1; }
  struct sockaddr_in me;
  memset(&me,0,sizeof(me));
  me.sin_family = AF_INET;
  me.sin_port = htons(PORT);
  me.sin_addr.s_addr = INADDR_ANY;
  if(bind(sock, (struct sockaddr*)&me, sizeof(me))<0){ perror("bind"); CLOSESOCKET(sock); platform_socket_cleanup(); return 1; }

  /* Initialize module queues and spawn pipeline threads so preproc/nn/represent/ui
     will actually run and process incoming messages (this causes creation of
     data/log_history.bin and data/nn_weights.bin). */
  queue_init(&raw_queue);
  queue_init(&proc_queue);
  queue_init(&repr_queue);
  queue_init(&error_queue);

  pthread_t t_preproc, t_nn, t_repr, t_ui;
  if(pthread_create(&t_preproc, NULL, preproc_thread, NULL) != 0){ perror("pthread_create preproc"); }
  if(pthread_create(&t_nn, NULL, nn_thread, NULL) != 0){ perror("pthread_create nn"); }
  if(pthread_create(&t_repr, NULL, represent_thread, NULL) != 0){ perror("pthread_create represent"); }
  if(pthread_create(&t_ui, NULL, ui_thread, NULL) != 0){ perror("pthread_create ui"); }

  printf("Simple receiver listening on UDP port %d (pipeline threads started)\n", PORT);

  while(1){
    char buf[8192];
    struct sockaddr_in from; socklen_t flen = sizeof(from);
    int n = (int)recvfrom(sock, buf, (int)sizeof(buf)-1, 0, (struct sockaddr*)&from, &flen);
    if(n <= 0) continue;
    if(n >= (int)sizeof(buf)) n = (int)sizeof(buf)-1;
    buf[n] = '\0';

    /* push the raw received line into the pipeline's raw queue. preproc will
       parse JSON or legacy CSV and persist history entries as appropriate. */
    queue_push(&raw_queue, buf);

    /* Also echo for debug */
    recv_msg_t m;
    memset(&m, 0, sizeof(m));
    if(buf[0] == '{'){
      strncpy(m.payload, buf, sizeof(m.payload)-1);
      m.payload[sizeof(m.payload)-1] = '\0';
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
          strncpy(m.payload, comma+1, sizeof(m.payload)-1);
          m.payload[sizeof(m.payload)-1] = '\0';
        }
      }
      if(!parsed_ts){
        strncpy(m.payload, buf, sizeof(m.payload)-1);
        m.payload[sizeof(m.payload)-1] = '\0';
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
    printf("--- received from %s:%d ---\n", m.src_addr, m.src_port);
    data_point_t dp; parse_json_to_datapoint(m.payload, &dp);
    int has_timestamp = !isnan(dp.timestamp);
    int has_export_bytes = !isnan(dp.export_bytes);
    if(has_timestamp || has_export_bytes || !isnan(dp.export_flows) || !isnan(dp.export_packets) || !isnan(dp.export_rtr) || !isnan(dp.export_rtt) || !isnan(dp.export_srt)){
      if(has_timestamp) m.ts = (long long)dp.timestamp;
      printf("timestamp: %lld\n", m.ts);
      printf("export_bytes: %.6f\n", dp.export_bytes);
      printf("export_flows: %.6f\n", dp.export_flows);
      printf("export_packets: %.6f\n", dp.export_packets);
      printf("export_rtr: %.6f\n", dp.export_rtr);
      printf("export_rtt: %.6f\n", dp.export_rtt);
      printf("export_srt: %.6f\n", dp.export_srt);
    } else {
      printf("timestamp: %lld\n", m.ts);
      printf("payload: %s\n", m.payload);
    }
    fflush(stdout);
  }

  CLOSESOCKET(sock);
  platform_socket_cleanup();
  return EXIT_SUCCESS;
}
