#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../queues.h"
#include "nn.h"
#include "nn_impl.h"
#include "nn_params.h"

#include <pthread.h>

void* nn_thread(void *arg){
    (void)arg;
    nn_params_t params = default_nn_params();
    nn_t* nn = nn_create(&params);
    if(!nn){ fprintf(stderr, "nn_create failed\n"); return NULL; }
    nn_load_weights(nn, "receiver/module2/nn_weights.bin");

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
        nn_predict_and_maybe_train(nn, &dp, NULL, out);

        char buf[256];
        int off = snprintf(buf, sizeof(buf), "pred");
        for(int i=0;i<OUTPUT_SIZE;i++) off += snprintf(buf+off, sizeof(buf)-off, ",%.6f", out[i]);
        queue_push(&repr_queue, buf);
        free(line);
    }
    nn_free(nn);
    return NULL;
}
