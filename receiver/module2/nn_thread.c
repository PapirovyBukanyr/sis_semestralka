#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../queues.h"
#include "nn.h"
#include "nn_impl.h"
#include "nn_params.h"

#include <pthread.h>
#include "../log.h"

void* nn_thread(void *arg){
    (void)arg;
    nn_params_t params = default_nn_params();
    nn_t* nn = nn_create(&params);
    if(!nn){ LOG_ERROR("nn_create failed\n"); return NULL; }
    /* load saved weights from canonical data directory if present */
    nn_load_weights(nn, "data/nn_weights.bin");

    int has_prev = 0;
    data_point_t prev_dp;
    float prev_out[OUTPUT_SIZE];
    double last_cost = NAN;

    while(1){
        char *line = queue_pop(&proc_queue);
        if(!line) break;
        // simple parse: CSV fields; first is ts, rest are floats (we'll map available values)
        double values[OUTPUT_SIZE];
        for(int i=0;i<OUTPUT_SIZE;i++) values[i]=0.0;
        int idx=0;
        char *tok = strtok(line, ",");
        while(tok != NULL && idx < OUTPUT_SIZE+1){
            if(idx==0){ /* ts */ }
            else { values[idx-1] = atof(tok); }
            idx++;
            tok = strtok(NULL, ",");
        }
        // build data_point_t from values (map order as in types.h)
        data_point_t dp;
        dp.timestamp = 0;
        dp.export_bytes = (float)values[0];
        dp.export_flows = (float)values[1];
        dp.export_packets = (float)values[2];
        dp.export_rtr = (float)values[3];
        dp.export_rtt = (float)values[4];
        dp.export_srt = (float)values[5];

        float out[OUTPUT_SIZE];

        /* If we have a previous datapoint, train the network using previous input -> current raw values
           as the target. The cost will be returned. */
        if(has_prev){
            float cur_raw[OUTPUT_SIZE];
            for(int i=0;i<OUTPUT_SIZE;i++) cur_raw[i] = (float)values[i];
            last_cost = nn_predict_and_maybe_train(nn, &prev_dp, cur_raw, prev_out);
            /* push the previous prediction, the actual target (current raw) and the cost for clarity */
            char pbuf[512];
            int poff = snprintf(pbuf, sizeof(pbuf), "pred_prev");
            for(int i=0;i<OUTPUT_SIZE;i++) poff += snprintf(pbuf+poff, sizeof(pbuf)-poff, ",pred,%.6f", prev_out[i]);
            for(int i=0;i<OUTPUT_SIZE;i++) poff += snprintf(pbuf+poff, sizeof(pbuf)-poff, ",target,%.6f", cur_raw[i]);
            poff += snprintf(pbuf+poff, sizeof(pbuf)-poff, ",cost,%.6f", (isnan(last_cost)?-1.0:last_cost));
            queue_push(&repr_queue, pbuf);
            /* build a single line and log it once (avoids interleaving) */
            char dbgbuf[512]; int dbgoff = snprintf(dbgbuf, sizeof(dbgbuf), "[nn] prev_pred vs target: ");
            for(int i=0;i<OUTPUT_SIZE && dbgoff < (int)sizeof(dbgbuf)-32;i++) dbgoff += snprintf(dbgbuf+dbgoff, sizeof(dbgbuf)-dbgoff, "p%.6f ", prev_out[i]);
            dbgoff += snprintf(dbgbuf+dbgoff, sizeof(dbgbuf)-dbgoff, " | ");
            for(int i=0;i<OUTPUT_SIZE && dbgoff < (int)sizeof(dbgbuf)-32;i++) dbgoff += snprintf(dbgbuf+dbgoff, sizeof(dbgbuf)-dbgoff, "t%.6f ", cur_raw[i]);
            dbgoff += snprintf(dbgbuf+dbgoff, sizeof(dbgbuf)-dbgoff, " | cost=%.6f\n", (isnan(last_cost)?-1.0:last_cost));
            LOG_INFO("%s", dbgbuf);
        }

        /* Predict for current datapoint (no target) to produce current prediction */
        double c = nn_predict_and_maybe_train(nn, &dp, NULL, out);
        (void)c;

        char buf[256];
        int off = snprintf(buf, sizeof(buf), "pred");
        for(int i=0;i<OUTPUT_SIZE;i++) off += snprintf(buf+off, sizeof(buf)-off, ",%.6f", out[i]);
        /* append last_cost for visibility (if available) */
        off += snprintf(buf+off, sizeof(buf)-off, ",cost,%.6f", (isnan(last_cost)?-1.0:last_cost));
        queue_push(&repr_queue, buf);

        /* store current as previous for next iteration */
        prev_dp = dp;
        has_prev = 1;
        free(line);
    }
    nn_free(nn);
    return NULL;
}
