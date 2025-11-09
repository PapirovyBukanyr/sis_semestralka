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
 * Periodically (every second) redraws a small status panel showing queue
 * lengths and the last error message. Errors are consumed via a non-blocking
 * pop so they are logged and shown but do not block the display refresh.
 */
void* ui_thread(void* arg){
    (void)arg;
    char *last_error = NULL;
    long long prev_received = 0, prev_processed = 0, prev_represented = 0;
    /* EMA smoothing state for per-second rates */
    const double ema_alpha = 0.3; /* smoothing factor: 0..1, higher = more responsive */
    double smooth_rps = 0.0, smooth_pps = 0.0, smooth_reps = 0.0;
    while(1){
        /* Drain any pending error messages (non-blocking) */
        char *e;
        while((e = queue_try_pop(&error_queue)) != NULL){
            if(last_error) free(last_error);
            last_error = e; /* take ownership */
            LOG_ERROR("[ui] error: %s\n", last_error);
        }

    /* Gather cumulative stats and per-second delta since last sample */
    long long tot_recv = 0, tot_proc = 0, tot_repr = 0;
    stats_get_counts(&tot_recv, &tot_proc, &tot_repr);
    long long rps = tot_recv - prev_received;
    long long pps = tot_proc - prev_processed;
    long long reps = tot_repr - prev_represented;
    prev_received = tot_recv; prev_processed = tot_proc; prev_represented = tot_repr;

    /* Update EMA-smoothed rates */
    smooth_rps = ema_alpha * (double)rps + (1.0 - ema_alpha) * smooth_rps;
    smooth_pps = ema_alpha * (double)pps + (1.0 - ema_alpha) * smooth_pps;
    smooth_reps = ema_alpha * (double)reps + (1.0 - ema_alpha) * smooth_reps;

    /* Gather windowed stats (last N seconds) and average prediction error */
    int window = STATS_WINDOW_SECONDS;
    long long w_recv=0, w_proc=0, w_repr=0;
    stats_get_window_rates(window, &w_recv, &w_proc, &w_repr);
    double avg_err = NAN;
    stats_get_avg_error(window, &avg_err);

    /* Clear screen and move cursor home using ANSI sequences. Most modern
     * terminals (including Windows 10+ terminals) support these. */
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

        /* Sleep 5 seconds in a cross-platform way */
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
