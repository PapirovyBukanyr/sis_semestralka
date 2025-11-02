\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../common.h"

extern str_queue_t repr_queue;
extern str_queue_t error_queue;

#define MAX_HISTORY 80

void clear_screen(){ printf("\033[2J\033[H"); fflush(stdout); }

void draw_graph(int history[], int n, int width){
    // simple ascii bar graph in top half
    int maxv = 1;
    for(int i=0;i<n;i++) if(history[i]>maxv) maxv = history[i];
    for(int row=0; row<10; row++){
        for(int i=0;i<n;i++){
            int h = (history[i]*10 + maxv-1)/maxv;
            if(h > 10) h = 10;
            if(10 - row <= h) printf("█"); else printf(" ");
        }
        printf("\n");
    }
}

void* ui_thread(void *arg){
    (void)arg;
    int hist[MAX_HISTORY]; memset(hist,0,sizeof(hist));
    int hist_n = 0;
    char *errors[1000]; int err_cnt=0;
    double last_acc = 0.0;
    char last_state[256] = "N/A";
    while(1){
        // non-blocking consume repr_queue and error_queue
        char *r = queue_pop(&repr_queue);
        while(r){
            long long ts; double score; char rec[128]; double acc;
            if(sscanf(r, "%lld,%lf,%127[^,],%lf", &ts, &score, rec, &acc) == 4){
                // append to history (use score>0.5 as a "count" proxy)
                int v = (score > 0.5) ? 1 : 0;
                if(hist_n < MAX_HISTORY) { hist[hist_n++]=v; }
                else {
                    memmove(hist, hist+1, sizeof(int)*(MAX_HISTORY-1));
                    hist[MAX_HISTORY-1] = v;
                }
                last_acc = acc;
                strncpy(last_state, rec, sizeof(last_state)-1);
                last_state[sizeof(last_state)-1]=0;
            } else {
                // push to errors store
                if(err_cnt < 1000) errors[err_cnt++] = r; else free(r);
            }
            r = queue_pop(&repr_queue);
        }
        char *e = queue_pop(&error_queue);
        while(e){
            if(err_cnt < 1000) errors[err_cnt++] = strdup(e);
            free(e);
            e = queue_pop(&error_queue);
        }

        // draw UI
        clear_screen();
        // Top half: graph
        printf("=== Top: logs over time (binary: anomaly=1) ===\n");
        draw_graph(hist, hist_n>0?hist_n:1, hist_n);
        // Bottom left
        printf("\n--- Left bottom: Current state & recommendation ---\n");
        printf("State: %s\n", last_state);
        printf("Prediction accuracy (running): %.2f%%\n", last_acc*100.0);
        // Bottom right: errors
        printf("\n--- Right bottom: Error log (recent) ---\n");
        int start = err_cnt - 10; if(start < 0) start = 0;
        for(int i=start;i<err_cnt;i++){
            printf("%s\n", errors[i]);
        }
        fflush(stdout);
        usleep(200000); // update ~5x per second
    }
    return NULL;
}
