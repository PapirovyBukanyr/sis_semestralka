/*
 * ui.c
 *
 * Minimal user interface / status reporting for the receiver application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../common.h"
#include "../queues.h"
#include "../log.h"
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Simple ASCII dashboard UI.
 *
 * arg: unused thread argument
 */
void* ui_thread(void* arg){
    (void)arg;
    char *last_error = NULL;
    long long prev_received = 0, prev_processed = 0, prev_represented = 0;
    const double ema_alpha = 0.3; 
    double smooth_rps = 0.0, smooth_pps = 0.0, smooth_reps = 0.0;
    while(1){
        char *e;
        while((e = queue_try_pop(&error_queue)) != NULL){
            if(last_error) free(last_error);
            last_error = e; 
            LOG_ERROR("[ui] error: %s\n", last_error);
        }

    long long tot_recv = 0, tot_proc = 0, tot_repr = 0;
    stats_get_counts(&tot_recv, &tot_proc, &tot_repr);
    long long rps = tot_recv - prev_received;
    long long pps = tot_proc - prev_processed;
    long long reps = tot_repr - prev_represented;
    prev_received = tot_recv; prev_processed = tot_proc; prev_represented = tot_repr;

    smooth_rps = ema_alpha * (double)rps + (1.0 - ema_alpha) * smooth_rps;
    smooth_pps = ema_alpha * (double)pps + (1.0 - ema_alpha) * smooth_pps;
    smooth_reps = ema_alpha * (double)reps + (1.0 - ema_alpha) * smooth_reps;

    int window = STATS_WINDOW_SECONDS;
    long long w_recv=0, w_proc=0, w_repr=0;
    stats_get_window_rates(window, &w_recv, &w_proc, &w_repr);
    double avg_err = NAN;
    stats_get_avg_error(window, &avg_err);

    printf("\x1b[2J\x1b[H");
        printf("+------------------------------------------------------+\n");
        printf("| Receiver UI - status                                 |\n");
        printf("+------------------------------------------------------+\n");
    printf(" Raw queue   : %4d   total: %lld   r/s: %.1f   win(%ds): %lld\n", queue_length(&raw_queue), tot_recv, smooth_rps, window, w_recv);
    printf(" Proc queue  : %4d   total: %lld   p/s: %.1f   win(%ds): %lld\n", queue_length(&proc_queue), tot_proc, smooth_pps, window, w_proc);
    printf(" Repr queue  : %4d   total: %lld   r/s: %.1f   win(%ds): %lld\n", queue_length(&repr_queue), tot_repr, smooth_reps, window, w_repr);
    printf(" Error queue : %4d\n", queue_length(&error_queue));
        printf("\n");
    if(isnan(avg_err)) printf(" Last error  : %s\n", last_error ? last_error : "(none)");
    else printf(" Avg pred abs err (last %ds): %.6f\n", window, avg_err);
        printf("\n");
        printf(" (UI updates every 5s; press Ctrl-C to quit)\n");
        fflush(stdout);

#ifdef _WIN32
        Sleep(5000);
#else
        struct timespec ts = {5, 0};
        nanosleep(&ts, NULL);
#endif
    }

    if(last_error) free(last_error);
    return NULL;
}
