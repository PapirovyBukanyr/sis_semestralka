\
/* SDL2 windowed UI replacement for previous terminal UI.
 * - Spawns two reader threads to consume repr_queue and error_queue (blocking).
 * - Main thread runs an SDL event loop, draws a line graph of recent prediction
 *   scores and shows the latest recommendation and running accuracy.
 *
 * Requires SDL2 development libraries available at build/link time. Makefile
 * is updated to link -lSDL2 on non-Windows builds.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../common.h"
#include "../queues.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Prefer SDL2 if available, otherwise fall back to terminal UI. */
#if defined(__has_include)
#  if __has_include(<SDL.h>)
#    include <SDL.h>
#    define HAVE_SDL 1
#  endif
#endif

#ifndef HAVE_SDL
/* If SDL not available, implement the previous terminal UI implementation below. */
#endif


// queue declarations provided by ../queues.h

#define WIN_W 800
#define WIN_H 600
#define GRAPH_H 360
#define MAX_HISTORY 512
#ifdef HAVE_SDL
static double hist[MAX_HISTORY];
static int hist_n = 0;
static pthread_mutex_t hist_m = PTHREAD_MUTEX_INITIALIZER;
static char last_state[256] = "N/A";
static double last_acc = 0.0;

/* simple ring buffer for recent error strings */
static char *errors[256]; static int err_head = 0; static int err_cnt = 0;
static pthread_mutex_t err_m = PTHREAD_MUTEX_INITIALIZER;

static void push_score(double score){
    pthread_mutex_lock(&hist_m);
    if(hist_n < MAX_HISTORY){ hist[hist_n++] = score; }
    else { memmove(hist, hist+1, sizeof(double)*(MAX_HISTORY-1)); hist[MAX_HISTORY-1] = score; }
    pthread_mutex_unlock(&hist_m);
}

static void push_error(const char *s){
    pthread_mutex_lock(&err_m);
    if(errors[err_head]){ free(errors[err_head]); }
    errors[err_head] = strdup(s);
    err_head = (err_head + 1) % 256;
    if(err_cnt < 256) err_cnt++; 
    pthread_mutex_unlock(&err_m);
}
#endif

#ifdef HAVE_SDL
static void *repr_reader(void *arg){
    (void)arg;
    while(1){
        char *r = queue_pop(&repr_queue);
        if(!r) break;
        long long ts; double score, acc; char rec[128];
        if(sscanf(r, "%lld,%lf,%127[^,],%lf", &ts, &score, rec, &acc) == 4){
            push_score(score);
            pthread_mutex_lock(&hist_m);
            last_acc = acc;
            pthread_mutex_unlock(&hist_m);
            pthread_mutex_lock(&err_m);
            strncpy(last_state, rec, sizeof(last_state)-1); last_state[sizeof(last_state)-1]=0;
            pthread_mutex_unlock(&err_m);
        } else {
            push_error(r);
        }
        free(r);
    }
    return NULL;
}

static void *error_reader(void *arg){
    (void)arg;
    while(1){
        char *e = queue_pop(&error_queue);
        if(!e) break;
        push_error(e);
        free(e);
    }
    return NULL;
}
#endif

#ifdef HAVE_SDL

static void draw_graph(SDL_Renderer *ren, int x, int y, int w, int h){
    pthread_mutex_lock(&hist_m);
    int n = hist_n;
    if(n <= 0){ pthread_mutex_unlock(&hist_m); return; }
    int maxp = n;
    double step = (double)w / (double)(maxp-1);
    SDL_SetRenderDrawColor(ren, 0x22,0x66,0xcc,255);
    for(int i=0;i<n-1;i++){
        int xi = x + (int)(i * step);
        int xj = x + (int)((i+1) * step);
        int yi = y + h - (int)(hist[i] * (double)h);
        int yj = y + h - (int)(hist[i+1] * (double)h);
        SDL_RenderDrawLine(ren, xi, yi, xj, yj);
    }
    pthread_mutex_unlock(&hist_m);
}

void* ui_thread(void *arg){
    (void)arg;
    if(SDL_Init(SDL_INIT_VIDEO) != 0){ fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return NULL; }
    SDL_Window *win = SDL_CreateWindow("Analyzer UI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, 0);
    if(!win){ fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); SDL_Quit(); return NULL; }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return NULL; }

    pthread_t tr, te;
    pthread_create(&tr, NULL, repr_reader, NULL);
    pthread_create(&te, NULL, error_reader, NULL);

    int running = 1;
    while(running){
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type == SDL_QUIT) running = 0;
        }

        /* clear */
        SDL_SetRenderDrawColor(ren, 0x10,0x10,0x10,255);
        SDL_RenderClear(ren);

        /* draw graph background */
        SDL_Rect grect = {20,20, WIN_W-40, GRAPH_H};
        SDL_SetRenderDrawColor(ren, 0x20,0x20,0x20,255);
        SDL_RenderFillRect(ren, &grect);

        /* draw grid lines */
        SDL_SetRenderDrawColor(ren, 0x33,0x33,0x33,255);
        for(int i=1;i<10;i++){
            int yy = 20 + i*(GRAPH_H/10);
            SDL_RenderDrawLine(ren, 20, yy, WIN_W-20, yy);
        }

        /* draw data line */
        SDL_SetRenderDrawColor(ren, 0xff,0xaa,0x33,255);
        draw_graph(ren, 20, 20, WIN_W-40, GRAPH_H);

        /* draw text-like info (simple rectangles and print to stderr for details) */
        // display last_state and last_acc as simple boxes
        SDL_SetRenderDrawColor(ren, 0x22,0x88,0x22,255);
        SDL_Rect info = {20, 20+GRAPH_H+20, WIN_W-40, 120};
        SDL_RenderFillRect(ren, &info);

        // We can't render TTF here without adding SDL_ttf; instead, print short info to stderr
        pthread_mutex_lock(&hist_m);
        double acc = last_acc; char state[256]; strncpy(state, last_state, sizeof(state));
        pthread_mutex_unlock(&hist_m);
        fprintf(stderr, "UI: Last state: %s | Accuracy: %.2f%%\n", state, acc*100.0);

        /* draw recent errors in the bottom-right as small boxes if any */
        pthread_mutex_lock(&err_m);
        int local_err_cnt = err_cnt;
        int idx = err_head;
        for(int i=0;i<local_err_cnt && i<8;i++){
            int j = (idx - 1 - i + 256) % 256;
            if(errors[j]){
                int ex = WIN_W - 380;
                int ey = 20+GRAPH_H+20 + 10 + i*12;
                // draw small red dot per error
                SDL_SetRenderDrawColor(ren, 0xaa,0x22,0x22,255);
                SDL_Rect d = {ex, ey, 6, 6}; SDL_RenderFillRect(ren, &d);
            }
        }
        pthread_mutex_unlock(&err_m);

        SDL_RenderPresent(ren);
        SDL_Delay(100); // ~10 fps
    }

    /* signal reader threads to exit by closing queues (optional). For now we detach/quit. */
    queue_close(&repr_queue);
    queue_close(&error_queue);
    pthread_join(tr, NULL);
    pthread_join(te, NULL);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return NULL;
}

#else /* fallback: terminal UI when SDL not available */

/* previous terminal UI implementation */
#define MAX_HISTORY_TERM 80

void clear_screen(){ printf("\033[2J\033[H"); fflush(stdout); }

void draw_graph_term(int history[], int n, int width){
    int maxv = 1;
    for(int i=0;i<n;i++) if(history[i]>maxv) maxv = history[i];
    for(int row=0; row<10; row++){
        for(int i=0;i<n;i++){
            int h = (history[i]*10 + maxv-1)/maxv;
            if(h > 10) h = 10;
            if(10 - row <= h) printf("#"); else printf(" ");
        }
        printf("\n");
    }
}

void* ui_thread(void *arg){
    (void)arg;
    int hist[MAX_HISTORY_TERM]; memset(hist,0,sizeof(hist));
    int hist_n_term = 0;
    char *errors_term[1000]; int err_cnt_term=0;
    double last_acc_term = 0.0;
    char last_state_term[256] = "N/A";
    while(1){
        char *r = queue_pop(&repr_queue);
        while(r){
            long long ts; double score; char rec[128]; double acc;
            if(sscanf(r, "%lld,%lf,%127[^,],%lf", &ts, &score, rec, &acc) == 4){
                int v = (score > 0.5) ? 1 : 0;
                if(hist_n_term < MAX_HISTORY_TERM) { hist[hist_n_term++]=v; }
                else {
                    memmove(hist, hist+1, sizeof(int)*(MAX_HISTORY_TERM-1));
                    hist[MAX_HISTORY_TERM-1] = v;
                }
                last_acc_term = acc;
                strncpy(last_state_term, rec, sizeof(last_state_term)-1);
                last_state_term[sizeof(last_state_term)-1]=0;
            } else {
                if(err_cnt_term < 1000) errors_term[err_cnt_term++] = r; else free(r);
            }
            r = queue_pop(&repr_queue);
        }
        char *e = queue_pop(&error_queue);
        while(e){
            if(err_cnt_term < 1000) errors_term[err_cnt_term++] = strdup(e);
            free(e);
            e = queue_pop(&error_queue);
        }

        clear_screen();
        printf("=== Top: logs over time (binary: anomaly=1) ===\n");
        draw_graph_term(hist, hist_n_term>0?hist_n_term:1, hist_n_term);
        printf("\n--- Left bottom: Current state & recommendation ---\n");
        printf("State: %s\n", last_state_term);
        printf("Prediction accuracy (running): %.2f%%\n", last_acc_term*100.0);
        printf("\n--- Right bottom: Error log (recent) ---\n");
        int start = err_cnt_term - 10; if(start < 0) start = 0;
        for(int i=start;i<err_cnt_term;i++){
            printf("%s\n", errors_term[i]);
        }
        fflush(stdout);
        /* portable sleep */
        #ifdef _WIN32
            Sleep(200);
        #else
            usleep(200000);
        #endif
    }
    return NULL;
}

#endif
