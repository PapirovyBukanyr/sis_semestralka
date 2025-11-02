\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../common.h"
#include "../module3/represent.h"

#define INPUT_N 2
#define HIDDEN_N 8
#define LR 0.001

static double w1[HIDDEN_N][INPUT_N];
static double b1[HIDDEN_N];
static double w2[HIDDEN_N];
static double b2;

static double last_pred = 0.0;
static double last_acc = 0.0;

extern str_queue_t proc_queue;

static double sigmoid(double x){ return 1.0 / (1.0 + exp(-x)); }

static void rand_init(){
    for(int i=0;i<HIDDEN_N;i++){
        b1[i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
        for(int j=0;j<INPUT_N;j++) w1[i][j] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
    for(int i=0;i<HIDDEN_N;i++) w2[i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    b2 = ((double)rand()/RAND_MAX - 0.5)*0.1;
}

static double forward(double in[INPUT_N], double hidden_out[HIDDEN_N]){
    for(int i=0;i<HIDDEN_N;i++){
        double s = b1[i];
        for(int j=0;j<INPUT_N;j++) s += w1[i][j]*in[j];
        hidden_out[i] = sigmoid(s);
    }
    double s = b2;
    for(int i=0;i<HIDDEN_N;i++) s += w2[i]*hidden_out[i];
    return sigmoid(s);
}

static void train_step(double in[INPUT_N], double target){
    double h[HIDDEN_N];
    double out = forward(in, h);
    double err = out - target; // derivative of loss (1/2*(out-target)^2)
    // output layer gradients
    double dout = err * out * (1-out);
    for(int i=0;i<HIDDEN_N;i++){
        double gw = dout * h[i];
        w2[i] -= LR * gw;
    }
    b2 -= LR * dout;
    // hidden layer grads
    for(int i=0;i<HIDDEN_N;i++){
        double dh = dout * w2[i];
        double grad = dh * h[i] * (1 - h[i]);
        for(int j=0;j<INPUT_N;j++){
            w1[i][j] -= LR * grad * in[j];
        }
        b1[i] -= LR * grad;
    }
    last_pred = out;
}

// very simple heuristic "label" generator for training: treat very high combined bytes as anomaly
static double heuristic_label(double in[INPUT_N]){
    double s = in[0] + in[1];
    // after normalization, threshold ~0.6
    return s > 0.6 ? 1.0 : 0.0;
}

double get_last_accuracy(){ return last_acc; }
double get_last_prediction(){ return last_pred; }

void* nn_thread(void *arg){
    (void)arg;
    srand((unsigned)time(NULL));
    rand_init();
    int total = 0;
    int correct = 0;
    while(1){
        char *line = queue_pop(&proc_queue);
        if(!line) break;
        // parse ts,bs,br
        long long ts; int bs, br;
        if(sscanf(line, "%lld,%d,%d", &ts, &bs, &br) != 3){
            free(line);
            continue;
        }
        // normalize simply by dividing by a rough max (e.g., 2000)
        double in[INPUT_N];
        in[0] = (double)bs / 2000.0;
        in[1] = (double)br / 2000.0;
        if(in[0] > 1.0) in[0]=1.0;
        if(in[1] > 1.0) in[1]=1.0;
        double target = heuristic_label(in);
        train_step(in, target);
        // evaluate
        double pred = get_last_prediction();
        int pred_label = pred > 0.5 ? 1 : 0;
        if(pred_label == (int)target) correct++;
        total++;
        last_acc = total>0 ? (double)correct/total : 0.0;
        // push to represent queue the prediction and ts
        char out[128];
        snprintf(out, sizeof(out), "%lld,%.6f,%.6f", ts, pred, last_acc);
        queue_push(&proc_queue, out); // reusing proc_queue as simple channel to represent; small hack
        free(line);
    }
    return NULL;
}
