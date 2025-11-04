#include "neuron.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Persistence filenames (placed in data/ to keep repo layout) */
#define NN_WEIGHTS_FILE "data/nn_weights.bin"

static double w1[HIDDEN_N][INPUT_N];
static double b1[HIDDEN_N];
static double w2[HIDDEN_N];
static double b2;

static double last_pred = 0.0;

static double sigmoid(double x){ return 1.0 / (1.0 + exp(-x)); }

void neuron_rand_init(void){
    for(int i=0;i<HIDDEN_N;i++){
        b1[i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
        for(int j=0;j<INPUT_N;j++) w1[i][j] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
    for(int i=0;i<HIDDEN_N;i++) w2[i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    b2 = ((double)rand()/RAND_MAX - 0.5)*0.1;
}

double neuron_forward(const double in[INPUT_N], double hidden_out[HIDDEN_N]){
    double hidden[HIDDEN_N];
    for(int i=0;i<HIDDEN_N;i++){
        double s = b1[i];
        for(int j=0;j<INPUT_N;j++) s += w1[i][j]*in[j];
        hidden[i] = sigmoid(s);
        if(hidden_out) hidden_out[i] = hidden[i];
    }
    double s = b2;
    for(int i=0;i<HIDDEN_N;i++) s += w2[i]*hidden[i];
    double out = sigmoid(s);
    last_pred = out;
    return out;
}

void neuron_train_step(const double in[INPUT_N], double target){
    double h[HIDDEN_N];
    double out = neuron_forward(in, h);
    double err = out - target;
    double dout = err * out * (1-out);
    for(int i=0;i<HIDDEN_N;i++){
        double gw = dout * h[i];
        w2[i] -= LR * gw;
    }
    b2 -= LR * dout;
    for(int i=0;i<HIDDEN_N;i++){
        double dh = dout * w2[i];
        double grad = dh * h[i] * (1 - h[i]);
        for(int j=0;j<INPUT_N;j++){
            w1[i][j] -= LR * grad * in[j];
        }
        b1[i] -= LR * grad;
    }
}

double neuron_get_last_prediction(void){ return last_pred; }

int neuron_save_weights(void){
    FILE *f = fopen(NN_WEIGHTS_FILE, "wb");
    if(!f) return -1;
    const char hdr[8] = "NNW1\0\0\0";
    if(fwrite(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { fclose(f); return -1; }
    for(int i=0;i<HIDDEN_N;i++) for(int j=0;j<INPUT_N;j++) if(fwrite(&w1[i][j], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    for(int i=0;i<HIDDEN_N;i++) if(fwrite(&b1[i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    for(int i=0;i<HIDDEN_N;i++) if(fwrite(&w2[i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    if(fwrite(&b2, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    fclose(f);
    return 0;
}

int neuron_load_weights(void){
    FILE *f = fopen(NN_WEIGHTS_FILE, "rb");
    if(!f) return -1;
    char hdr[8];
    if(fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)){ fclose(f); return -1; }
    if(strncmp(hdr, "NNW1", 4) != 0){ fclose(f); return -1; }
    for(int i=0;i<HIDDEN_N;i++) for(int j=0;j<INPUT_N;j++) if(fread(&w1[i][j], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    for(int i=0;i<HIDDEN_N;i++) if(fread(&b1[i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    for(int i=0;i<HIDDEN_N;i++) if(fread(&w2[i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    if(fread(&b2, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    fclose(f);
    return 0;
}
