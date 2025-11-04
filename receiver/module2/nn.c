\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../common.h"
#include "../queues.h"
#include "../module3/represent.h"
#include "neuron.h"
#include "history.h"
#include <errno.h>
#include <stdint.h>

#define SAVE_WEIGHTS_EVERY 500

/* helper: load all history entries into dynamically allocated array; returns count via out_n
   caller should free returned pointer. */
/* history_load_all implemented in module2/history.c */

/* proc_queue declared in ../queues.h */

static double last_acc = 0.0;

void* nn_thread(void *arg){
    (void)arg;
    /* Try loading previously trained weights; otherwise initialize randomly */
    neuron_rand_init();
    if(neuron_load_weights() != 0){
        /* layer initialization will be random already */
    }
    int total = 0;
    int correct = 0;
    /* Load history and optionally perform an initial sweep of training */
    size_t hist_n = 0;
    history_entry_t *hist = history_load_all(&hist_n);
    if(hist && hist_n > 0){
        for(size_t i=0;i<hist_n;i++){
            double in_init[INPUT_N]; in_init[0] = hist[i].in0; in_init[1] = hist[i].in1;
            /* simple local heuristic label */
            double s = in_init[0] + in_init[1];
            double target = s > 0.6 ? 1.0 : 0.0;
            neuron_train_step(in_init, target);
        }
        free(hist);
    }
    int save_counter = 0;

    while(1){
        char *line = queue_pop(&proc_queue);
        if(!line) break;
        /* The pipeline can produce two kinds of normalized lines:
           1) JSON-based normalized CSV (from convertor):
              timestamp,export_bytes,export_flows,export_packets,export_rtr,export_rtt,export_srt
           2) Legacy CSV: timestamp,bs,br
           Try to parse the new format first and fall back to legacy parsing.
        */
        long long ts = 0;
        double export_bytes = NAN, export_flows = NAN, export_packets = NAN, export_rtr = NAN, export_rtt = NAN, export_srt = NAN;
        int parsed_new = 0;
        if(sscanf(line, "%lld,%lf,%lf,%lf,%lf,%lf,%lf", &ts, &export_bytes, &export_flows, &export_packets, &export_rtr, &export_rtt, &export_srt) == 7){
            parsed_new = 1;
        }
        double in[INPUT_N];
        if(parsed_new){
            /* Normalize new-format inputs. Choose export_bytes and export_flows as NN inputs.
               Scaling: export_bytes can be large (up to millions), so divide by 1e6.
               export_flows typically small, divide by 100. Clamp to [0,1]. */
            in[0] = export_bytes / 1000000.0; if(isnan(in[0]) || in[0] < 0) in[0] = 0.0; if(in[0] > 1.0) in[0] = 1.0;
            in[1] = export_flows / 100.0; if(isnan(in[1]) || in[1] < 0) in[1] = 0.0; if(in[1] > 1.0) in[1] = 1.0;
        } else {
            int bs = -1, br = -1;
            if(sscanf(line, "%lld,%d,%d", &ts, &bs, &br) != 3){
                free(line);
                continue;
            }
            /* legacy normalization retained */
            in[0] = (double)bs / 2000.0; if(in[0] < 0) in[0] = 0.0; if(in[0] > 1.0) in[0] = 1.0;
            in[1] = (double)br / 2000.0; if(in[1] < 0) in[1] = 0.0; if(in[1] > 1.0) in[1] = 1.0;
        }
        if(in[0] > 1.0) in[0]=1.0;
        if(in[1] > 1.0) in[1]=1.0;
        double s = in[0] + in[1];
        double target = s > 0.6 ? 1.0 : 0.0;
        neuron_train_step(in, target);
        // evaluate
        double pred = neuron_get_last_prediction();
        int pred_label = pred > 0.5 ? 1 : 0;
        if(pred_label == (int)target) correct++;
        total++;
        last_acc = total>0 ? (double)correct/total : 0.0;
    /* periodically save weights */
    if(++save_counter >= SAVE_WEIGHTS_EVERY){ neuron_save_weights(); save_counter = 0; }

        // push to represent queue the prediction and ts
        char out[128];
     snprintf(out, sizeof(out), "%lld,%.6f,%.6f", ts, pred, last_acc);
             /* debug: also print the raw prediction line so analyzer process shows NN output
                 directly in the terminal (helps when rebuilding or when UI is not used) */
             printf("PRED_OUT:%s\n", out);
             fflush(stdout);
             /* push prediction back into proc_queue so represent_thread (which reads proc_queue)
                 will consume it and produce the textual representation placed into repr_queue */
             queue_push(&proc_queue, out);
        free(line);
    }
    /* final save on thread exit */
    neuron_save_weights();
    return NULL;
}
