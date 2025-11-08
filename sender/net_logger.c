/*
 CSV sender: reads CSV files from a directory (default "data") or a single file
 and sends each CSV row over UDP to SERVER_IP:SERVER_PORT.

 Usage: net_logger [data_path]
   - If data_path is a file, that file is sent.
   - If data_path is a directory, all *.csv files inside are sent in lexicographic order.

 Timing: CSV rows are expected as `timestamp,<value>` where timestamp is unix seconds.
 The sender waits between rows by (ts_next - ts_prev) / accel seconds (accel defaults to 10.0).
 After all files are sent, the sender loops and repeats.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  typedef SOCKET socket_t;
  static int socket_init(void){ WSADATA w; return (WSAStartup(MAKEWORD(2,2), &w) == 0) ? 0 : -1; }
  static void socket_cleanup(void){ WSACleanup(); }
  static long long now_ms(void){
    FILETIME ft; GetSystemTimeAsFileTime(&ft);
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    const unsigned long long EPOCH_DIFF = 116444736000000000ULL;
    return (long long)((t - EPOCH_DIFF) / 10000ULL);
  }
  static void sleep_us(unsigned int usec){ Sleep((usec + 999) / 1000); }
  static void close_socket(socket_t s){ closesocket(s); }

  /* Windows: list *.csv files inside directory */
  static int list_csv_files(const char *dir, char ***out_files, size_t *out_n){
    char pattern[1024]; size_t dlen = strlen(dir);
    if(dlen + 8 >= sizeof(pattern)) return -1;
    strcpy(pattern, dir);
    if(dlen && (pattern[dlen-1] != '\\' && pattern[dlen-1] != '/')) strcat(pattern, "\\");
    strcat(pattern, "*.csv");
    WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pattern, &fd);
    if(h == INVALID_HANDLE_VALUE) return 0;
    char **files = NULL; size_t n = 0;
    do{
      if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        size_t pathlen = dlen + 1 + strlen(fd.cFileName) + 1;
        char *p = malloc(pathlen);
        if(!p) break;
        if(dlen && (dir[dlen-1] != '/' && dir[dlen-1] != '\\')) snprintf(p, pathlen, "%s\\%s", dir, fd.cFileName);
        else snprintf(p, pathlen, "%s%s", dir, fd.cFileName);
        char **tmp = realloc(files, sizeof(char*)*(n+1)); if(!tmp){ free(p); break; }
        files = tmp; files[n++] = p;
      }
    } while(FindNextFileA(h, &fd));
    FindClose(h);
    *out_files = files; *out_n = n; return 0;
  }

#else
  #include <unistd.h>
  #include <sys/time.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <dirent.h>
  typedef int socket_t;
  static int socket_init(void){ return 0; }
  static void socket_cleanup(void){}
  static long long now_ms(void){ struct timeval tv; gettimeofday(&tv, NULL); return (long long)tv.tv_sec * 1000 + tv.tv_usec/1000; }
  static void sleep_us(unsigned int usec){ usleep(usec); }
  static void close_socket(socket_t s){ close(s); }

  /* POSIX: list *.csv files in directory */
  static int list_csv_files(const char *dir, char ***out_files, size_t *out_n){
    DIR *d = opendir(dir); if(!d) return -1;
    struct dirent *ent; char **files = NULL; size_t n = 0;
    while((ent = readdir(d))){
      if(ent->d_type == DT_DIR) continue;
      const char *name = ent->d_name; size_t len = strlen(name);
      if(len > 4 && strcmp(name+len-4, ".csv") == 0){
        size_t pathlen = strlen(dir) + 1 + len + 1; char *p = malloc(pathlen);
        if(!p) break;
        if(dir[strlen(dir)-1] == '/') snprintf(p, pathlen, "%s%s", dir, name); else snprintf(p, pathlen, "%s/%s", dir, name);
        char **tmp = realloc(files, sizeof(char*)*(n+1)); if(!tmp){ free(p); break; }
        files = tmp; files[n++] = p;
      }
    }
    closedir(d); *out_files = files; *out_n = n; return 0;
  }
#endif

static void free_files(char **files, size_t n){ if(!files) return; for(size_t i=0;i<n;++i) free(files[i]); free(files); }

/* simple comparator for qsort to sort filenames lexicographically */
static int cmpstr(const void *a, const void *b){ const char *A = *(const char**)a; const char *B = *(const char**)b; return strcmp(A,B); }

int main(int argc, char **argv){
  if(socket_init() != 0){ fprintf(stderr, "socket init failed\n"); return 1; }

  socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock == (socket_t)-1 || sock == INVALID_SOCKET){ perror("socket"); socket_cleanup(); return 1; }

  struct sockaddr_in srv; memset(&srv,0,sizeof(srv)); srv.sin_family = AF_INET; srv.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP, &srv.sin_addr);

  /* parse simple CLI flags: -a <accel>, -1/--once, -s/--append-source */
  double accel = 50.0;
  int once = 0;
  int append_source = 0;
  int json_mode = 0;
  const char *json_path = NULL;
  double rate = 10000; /* packets per second for json mode */
  const char *src = "data";
  for(int i=1;i<argc;i++){
    if(strcmp(argv[i], "-a")==0 || strcmp(argv[i], "--accel")==0){ if(i+1<argc) { accel = atof(argv[++i]); continue; } }
    if(strcmp(argv[i], "-1")==0 || strcmp(argv[i], "--once")==0){ once = 1; continue; }
    if(strcmp(argv[i], "-s")==0 || strcmp(argv[i], "--append-source")==0){ append_source = 1; continue; }
    if(strcmp(argv[i], "-j")==0 || strcmp(argv[i], "--json")==0){ if(i+1<argc) { json_mode = 1; json_path = argv[++i]; continue; } }
    if(strcmp(argv[i], "-r")==0 || strcmp(argv[i], "--rate")==0){ if(i+1<argc) { rate = atof(argv[++i]); if(rate <= 0) rate = 1.0; continue; } }
    src = argv[i];
  }
  if(accel <= 0) accel = 10.0;
  printf("Starting net logger -> %s:%d (accelerated x%.1f)\n", SERVER_IP, SERVER_PORT, accel);
  char **files = NULL; size_t nfiles = 0;

  /* If user didn't explicitly pick a different source, prefer data/merged.jsonl when present.
     Try a few candidate paths so running the binary from bin/ works (e.g. ../data/merged.jsonl). */
  if(!json_mode && strcmp(src, "data") == 0){
    const char *cands[] = { "data/merged.jsonl", "../data/merged.jsonl", "..\\data\\merged.jsonl", "./data/merged.jsonl" };
    for(size_t ci=0; ci<sizeof(cands)/sizeof(cands[0]); ++ci){
      const char *cand = cands[ci];
      FILE *jf = fopen(cand, "r");
      if(jf){ fclose(jf); json_mode = 1; json_path = cand; printf("Auto JSON mode enabled using %s\n", cand); break; }
    }
  }
  FILE *fcheck = fopen(src, "r");
  if(fcheck){ fclose(fcheck); files = malloc(sizeof(char*)); files[0] = strdup(src); nfiles = 1; }
  else {
    if(list_csv_files(src, &files, &nfiles) != 0 || nfiles == 0){ fprintf(stderr, "No CSV files found in '%s' or cannot open\n", src); close_socket(sock); socket_cleanup(); free_files(files,nfiles); return 1; }
  }

  /* sort file list for stable order */
  if(nfiles > 1) qsort(files, nfiles, sizeof(char*), cmpstr);
  /* Merge-mode: open all files and send rows in global timestamp order */
  /* If json mode requested, read jsonl file and send at specified rate */
  if(json_mode){
    const char *jpath = json_path ? json_path : "data/merged.jsonl";
    while(1){
      FILE *jf = fopen(jpath, "r");
      if(!jf){ fprintf(stderr, "Cannot open JSON file %s: %s\n", jpath, strerror(errno)); break; }
      char line[8192];
      while(fgets(line, sizeof(line), jf)){
        size_t L = strlen(line); while(L>0 && (line[L-1]=='\n' || line[L-1]=='\r')) line[--L]=0;
        if(L==0) continue;
        char outbuf[16384]; if(append_source) snprintf(outbuf, sizeof(outbuf), "%s,%s", line, jpath); else { strncpy(outbuf, line, sizeof(outbuf)); outbuf[sizeof(outbuf)-1]=0; }
        sendto(sock, outbuf, (int)strlen(outbuf), 0, (struct sockaddr*)&srv, sizeof(srv));
        printf("sent: %s\n", outbuf);
        unsigned int usec = (unsigned int)(1e6 / rate);
        if(usec) sleep_us(usec);
      }
      fclose(jf);
      if(once) break;
      /* otherwise loop and re-open to repeat */
    }
    close_socket(sock);
    socket_cleanup();
    free_files(files, nfiles);
    return 0;
  }

  while(1){
    FILE **fhs = calloc(nfiles, sizeof(FILE*));
    char **curline = calloc(nfiles, sizeof(char*));
    long long *curts = calloc(nfiles, sizeof(long long));
    int *active = calloc(nfiles, sizeof(int));
    if(!fhs || !curline || !curts || !active){ fprintf(stderr, "alloc failed\n"); break; }

    /* open files and read first data row for each */
    for(size_t i=0;i<nfiles;++i){
      fhs[i] = fopen(files[i], "r");
      if(!fhs[i]){ fprintf(stderr, "Cannot open %s: %s\n", files[i], strerror(errno)); active[i]=0; continue; }
      char buf[4096]; int first = 1; long long ts = -1; char *saved = NULL;
      while(fgets(buf, sizeof(buf), fhs[i])){
        size_t L = strlen(buf); while(L>0 && (buf[L-1]=='\n' || buf[L-1]=='\r')) buf[--L]=0; if(L==0) continue;
        if(first){ if(strncmp(buf, "timestamp", 9)==0){ first = 0; continue; } first = 0; }
        char *comma = strchr(buf, ','); if(!comma) continue;
        char tsbuf[64]; size_t tlen = (size_t)(comma - buf); if(tlen >= sizeof(tsbuf)) tlen = sizeof(tsbuf)-1; memcpy(tsbuf, buf, tlen); tsbuf[tlen]=0;
        ts = atoll(tsbuf);
        saved = strdup(buf);
        break;
      }
      if(saved){ curline[i] = saved; curts[i] = ts; active[i]=1; }
      else { fclose(fhs[i]); fhs[i]=NULL; active[i]=0; }
    }

    long long prev_sent_ts = -1;
    int any_active = 0; for(size_t i=0;i<nfiles;++i) if(active[i]) { any_active=1; break; }
    if(!any_active){ fprintf(stderr, "No data found in files\n"); free(fhs); free(curline); free(curts); free(active); break; }

    /* repeatedly pick smallest timestamp among active rows */
    while(1){
      /* find min ts */
      long long min_ts = LLONG_MAX; int min_idx = -1;
      for(size_t i=0;i<nfiles;++i) if(active[i]){ if(curts[i] < min_ts){ min_ts = curts[i]; min_idx = (int)i; } }
      if(min_idx == -1) break; /* all done */

      /* wait by delta from prev_sent_ts */
      if(prev_sent_ts != -1){ long long dt = min_ts - prev_sent_ts; if(dt > 0){ double wait_sec = (double)dt / accel; unsigned int usec = (unsigned int)(wait_sec * 1e6); if(usec) sleep_us(usec); } }
      prev_sent_ts = min_ts;

      /* send curline[min_idx] */
      char outbuf[8192]; if(append_source) snprintf(outbuf, sizeof(outbuf), "%s,%s", curline[min_idx], files[min_idx]); else { strncpy(outbuf, curline[min_idx], sizeof(outbuf)); outbuf[sizeof(outbuf)-1]=0; }
      sendto(sock, outbuf, (int)strlen(outbuf), 0, (struct sockaddr*)&srv, sizeof(srv));
      printf("sent: %s\n", outbuf);

      /* advance that file: read next data row or mark inactive */
      free(curline[min_idx]); curline[min_idx]=NULL; curts[min_idx]=0;
      char buf2[4096]; long long ts2=-1; int found=0;
      while(fhs[min_idx] && fgets(buf2, sizeof(buf2), fhs[min_idx])){
        size_t L = strlen(buf2); while(L>0 && (buf2[L-1]=='\n' || buf2[L-1]=='\r')) buf2[--L]=0; if(L==0) continue;
        if(strncmp(buf2, "timestamp", 9)==0) continue;
        char *comma = strchr(buf2, ','); if(!comma) continue;
        char tsbuf[64]; size_t tlen = (size_t)(comma - buf2); if(tlen >= sizeof(tsbuf)) tlen = sizeof(tsbuf)-1; memcpy(tsbuf, buf2, tlen); tsbuf[tlen]=0;
        ts2 = atoll(tsbuf);
        curline[min_idx] = strdup(buf2);
        curts[min_idx] = ts2;
        found = 1; break;
      }
      if(!found){ if(fhs[min_idx]){ fclose(fhs[min_idx]); fhs[min_idx]=NULL; } active[min_idx]=0; }
      /* loop continues picking next min */
    }

    /* cleanup file handles and buffers */
    for(size_t i=0;i<nfiles;++i){ if(fhs[i]) fclose(fhs[i]); if(curline[i]) free(curline[i]); }
    free(fhs); free(curline); free(curts); free(active);

    if(once) break; /* only one merged pass */
    /* else loop and restart (will re-open files) */
  }
  free_files(files, nfiles);
  close_socket(sock);
  socket_cleanup();
  return 0;
}
 
