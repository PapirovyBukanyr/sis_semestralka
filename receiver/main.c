/*
 Receiver / Analyzer program.
 Listens on UDP port 9000, enqueues raw logs to raw_queue.
 Spawns threads:
  - preproc_thread (module1)
  - nn_thread (module2)
  - represent_thread (module3)
  - ui_thread (module4)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform-independent socket compatibility added to support Windows (no arpa/inet.h) */
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

#include "common.h"
#include "module1/preproc.h"
#include "module2/nn.h"
#include "module3/represent.h"
#include "module4/ui.h"
#include <math.h>
/* Portable socket typedefs and helpers */
#ifdef _WIN32
  /* winsock2 included earlier */
  typedef SOCKET socket_t;
  typedef int socklen_t;
  #define CLOSESOCKET(s) closesocket(s)
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (SOCKET)(~0)
  #endif
#else
  typedef int socket_t;
  #define CLOSESOCKET(s) close(s)
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET -1
  #endif
#endif

/* Initialize/cleanup sockets on Windows, no-op on POSIX */
static int platform_socket_init(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif
    return 0;
}

static void platform_socket_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

str_queue_t raw_queue;
str_queue_t proc_queue;
str_queue_t repr_queue;
str_queue_t error_queue;

/* Minimal message structure used for printing received data */
typedef struct {
  long long ts;   /* timestamp, either parsed from message or receive time (ms) */
  char payload[8192]; /* raw payload (truncated if larger) */
  char src_addr[64];
  int src_port;
} recv_msg_t;

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

/* Parse a simple flat JSON object with numeric fields into data_point_t */
static void parse_json_to_datapoint(const char *s, data_point_t *d){
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

int main(int argc, char **argv) {
  (void)argc; (void)argv;
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

  printf("Simple receiver listening on UDP port %d\n", PORT);

  while(1){
    char buf[8192];
    struct sockaddr_in from; socklen_t flen = sizeof(from);
    int n = (int)recvfrom(sock, buf, (int)sizeof(buf)-1, 0, (struct sockaddr*)&from, &flen);
    if(n <= 0) continue;
    if(n >= (int)sizeof(buf)) n = (int)sizeof(buf)-1;
    buf[n] = '\0';

    recv_msg_t m;
    memset(&m, 0, sizeof(m));
    /* try parse payload: prefer JSON (starts with '{'), otherwise accept CSV with leading timestamp */
    if(buf[0] == '{'){
      /* JSON whole payload */
      strncpy(m.payload, buf, sizeof(m.payload)-1);
      m.payload[sizeof(m.payload)-1] = '\0';
      m.ts = 0;
    } else {
      char *comma = strchr(buf, ',');
      int parsed_ts = 0;
      if(comma){
        /* check that chars before comma look like a numeric timestamp */
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
        /* fallback: copy full payload and set ts to receive time (seconds) */
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

    /* fill source address */
    inet_ntop(AF_INET, &from.sin_addr, m.src_addr, sizeof(m.src_addr));
    m.src_port = ntohs(from.sin_port);

    /* Print the parsed structure to terminal. Try parsing JSON-like numeric fields
       for any payload (some senders may strip braces or add leading chars). If
       parsing yields any numeric values, print them; otherwise fall back to raw
       payload printing. */
    printf("--- received from %s:%d ---\n", m.src_addr, m.src_port);
    data_point_t dp; parse_json_to_datapoint(m.payload, &dp);
    /* check if parsing produced at least one numeric value (timestamp or export_bytes) */
    int has_timestamp = !isnan(dp.timestamp);
    int has_export_bytes = !isnan(dp.export_bytes);
    if(has_timestamp || has_export_bytes || !isnan(dp.export_flows) || !isnan(dp.export_packets) || !isnan(dp.export_rtr) || !isnan(dp.export_rtt) || !isnan(dp.export_srt)){
      /* if JSON had a timestamp, use it for m.ts (seconds)
         otherwise keep previously-determined m.ts */
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
