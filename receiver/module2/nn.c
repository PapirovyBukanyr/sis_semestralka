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

    /* local circular buffer to assemble sequences of length SEQ_LEN */
    double seq_buf[SEQ_LEN][INPUT_N];
    int seq_pos = 0;
    int seq_cnt = 0;

    while(1){
        char *line = queue_pop(&proc_queue);
        if(!line) break;
        long long ts = 0;
        double export_bytes = NAN, export_flows = NAN, export_packets = NAN, export_rtr = NAN, export_rtt = NAN, export_srt = NAN;
        int parsed_new = 0;
        if(sscanf(line, "%lld,%lf,%lf,%lf,%lf,%lf,%lf", &ts, &export_bytes, &export_flows, &export_packets, &export_rtr, &export_rtt, &export_srt) == 7){
            parsed_new = 1;
        }
        double in[INPUT_N];
        if(parsed_new){
            in[0] = export_bytes / 1000000.0; if(isnan(in[0]) || in[0] < 0) in[0] = 0.0; if(in[0] > 1.0) in[0] = 1.0;
            in[1] = export_flows / 100.0; if(isnan(in[1]) || in[1] < 0) in[1] = 0.0; if(in[1] > 1.0) in[1] = 1.0;
        } else {
            int bs = -1, br = -1;
            if(sscanf(line, "%lld,%d,%d", &ts, &bs, &br) != 3){
                free(line);
                continue;
            }
            in[0] = (double)bs / 2000.0; if(in[0] < 0) in[0] = 0.0; if(in[0] > 1.0) in[0] = 1.0;
            in[1] = (double)br / 2000.0; if(in[1] < 0) in[1] = 0.0; if(in[1] > 1.0) in[1] = 1.0;
        }
        if(in[0] > 1.0) in[0]=1.0;
        if(in[1] > 1.0) in[1]=1.0;

        /* push into local sequence buffer */
        memcpy(seq_buf[seq_pos], in, sizeof(double)*INPUT_N);
        seq_pos = (seq_pos + 1) % SEQ_LEN;
        if(seq_cnt < SEQ_LEN) seq_cnt++;
        if(seq_cnt < SEQ_LEN){ free(line); continue; } /* need more history */

        /* build concatenated sequence: oldest..newest (length SEQ_LEN) */
        double seq_in[INPUT_N * SEQ_LEN];
        int base = seq_pos;
        for(int s=0;s<SEQ_LEN;s++){
            int idx = (base + s) % SEQ_LEN;
            for(int k=0;k<INPUT_N;k++) seq_in[s*INPUT_N + k] = seq_buf[idx][k];
        }
        /* target vector is the last element in the sequence (we predict next vector; here train to reproduce last as proxy) */
        double target_vec[INPUT_N];
        for(int k=0;k<INPUT_N;k++) target_vec[k] = seq_in[(SEQ_LEN-1)*INPUT_N + k];

        neuron_train_step_seq(seq_in, target_vec);

        /* predict */
        double pred_vec[INPUT_N];
        neuron_predict_seq(seq_in, pred_vec);

        /* simple evaluation: consider correct if first component sign matches target threshold */
        double pred = pred_vec[0];
        int pred_label = pred > 0.5 ? 1 : 0;
        double s = seq_in[(SEQ_LEN-1)*INPUT_N + 0] + seq_in[(SEQ_LEN-1)*INPUT_N + 1];
        double target_label = s > 0.6 ? 1.0 : 0.0;
        if(pred_label == (int)target_label) correct++;
        total++;
        last_acc = total>0 ? (double)correct/total : 0.0;

        if(++save_counter >= SAVE_WEIGHTS_EVERY){ neuron_save_weights(); save_counter = 0; }

        /* push to represent queue the prediction vector and accuracy */
        char out[256];
        /* format: ts, pred0, pred1, acc */
        snprintf(out, sizeof(out), "%lld,%.6f,%.6f,%.6f", ts, pred_vec[0], pred_vec[1], last_acc);
        printf("PRED_OUT:%s\n", out);
        fflush(stdout);
        queue_push(&proc_queue, strdup(out));
        free(line);
    }
    /* final save on thread exit */
    neuron_save_weights();
    return NULL;
}
