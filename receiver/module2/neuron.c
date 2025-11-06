/* Intentionally left minimal: neural implementation lives in neuron_layer.c.
   This file exists to satisfy the user's requested layout (neuron.c present).
   No symbols are defined here to avoid duplicate definitions at link time. */

#include "neuron.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Persistence filenames (placed in data/ to keep repo layout) */
#define NN_WEIGHTS_FILE "data/nn_weights.bin"

/* Hidden layers count and size */
#define H_LAYERS 10
#define H_SZ 5

static double last_pred_scalar = 0.0; /* legacy single-value API: store first component */
static double last_pred_vec[INPUT_N];

/* Weights and biases:
   - input -> first hidden: w_input[H_SZ][INPUT_N * SEQ_LEN]
   - hidden layers: w_hidden[layer][H_SZ][H_SZ]
   - biases: b_hidden[layer][H_SZ]
   - output: w_out[INPUT_N][H_SZ], b_out[INPUT_N]
*/
static double w_input[H_SZ][INPUT_N * SEQ_LEN];
static double b_hidden[H_LAYERS][H_SZ];
static double w_hidden[H_LAYERS-1][H_SZ][H_SZ];
static double w_out[INPUT_N][H_SZ];
static double b_out[INPUT_N];

/* internal sequence memory (legacy wrapper compatibility) */
static double mem_buf[SEQ_LEN][INPUT_N];
static int mem_cnt = 0;
static int mem_pos = 0;

static double sigmoid(double x){ return 1.0 / (1.0 + exp(-x)); }

void neuron_rand_init(void){
    for(int i=0;i<H_SZ;i++){
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) w_input[i][j] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
    for(int l=0;l<H_LAYERS;l++){
        for(int i=0;i<H_SZ;i++) b_hidden[l][i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
    for(int l=0;l<H_LAYERS-1;l++){
        for(int i=0;i<H_SZ;i++) for(int j=0;j<H_SZ;j++) w_hidden[l][i][j] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
    for(int k=0;k<INPUT_N;k++){
        for(int i=0;i<H_SZ;i++) w_out[k][i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
        b_out[k] = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
}

/* Predict next vector from a concatenated sequence input of length INPUT_N * SEQ_LEN
   out is an array of length INPUT_N. */
void neuron_predict_seq(const double in_seq[INPUT_N * SEQ_LEN], double out[INPUT_N]){
    double hidden[H_LAYERS][H_SZ];
    /* first hidden layer */
    for(int i=0;i<H_SZ;i++){
        double s = b_hidden[0][i];
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) s += w_input[i][j] * in_seq[j];
        hidden[0][i] = sigmoid(s);
    }
    /* deeper hidden layers */
    for(int l=1;l<H_LAYERS;l++){
        for(int i=0;i<H_SZ;i++){
            double s = b_hidden[l][i];
            for(int j=0;j<H_SZ;j++) s += w_hidden[l-1][i][j] * hidden[l-1][j];
            hidden[l][i] = sigmoid(s);
        }
    }
    /* output layer (vector) */
    for(int k=0;k<INPUT_N;k++){
        double s = b_out[k];
        for(int i=0;i<H_SZ;i++) s += w_out[k][i] * hidden[H_LAYERS-1][i];
        out[k] = sigmoid(s);
        last_pred_vec[k] = out[k];
    }
    last_pred_scalar = last_pred_vec[0]; /* legacy */
}

/* Train on a single sequence -> target vector pair (online SGD). */
void neuron_train_step_seq(const double in_seq[INPUT_N * SEQ_LEN], const double target[INPUT_N]){
    double hidden[H_LAYERS][H_SZ];
    /* compute hidden states */
    for(int i=0;i<H_SZ;i++){
        double s = b_hidden[0][i];
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) s += w_input[i][j] * in_seq[j];
        hidden[0][i] = sigmoid(s);
    }
    for(int l=1;l<H_LAYERS;l++){
        for(int i=0;i<H_SZ;i++){
            double s = b_hidden[l][i];
            for(int j=0;j<H_SZ;j++) s += w_hidden[l-1][i][j] * hidden[l-1][j];
            hidden[l][i] = sigmoid(s);
        }
    }
    double out[INPUT_N];
    for(int k=0;k<INPUT_N;k++){
        double s = b_out[k];
        for(int i=0;i<H_SZ;i++) s += w_out[k][i] * hidden[H_LAYERS-1][i];
        out[k] = sigmoid(s);
    }
    /* output layer gradients */
    double dout[INPUT_N];
    for(int k=0;k<INPUT_N;k++) dout[k] = (out[k] - target[k]) * out[k] * (1 - out[k]);
    /* update output weights */
    for(int k=0;k<INPUT_N;k++){
        for(int i=0;i<H_SZ;i++) w_out[k][i] -= LR * dout[k] * hidden[H_LAYERS-1][i];
        b_out[k] -= LR * dout[k];
    }
    /* backprop through hidden layers */
    double delta[H_LAYERS][H_SZ];
    /* last hidden layer */
    for(int i=0;i<H_SZ;i++){
        double dh = 0.0;
        for(int k=0;k<INPUT_N;k++) dh += dout[k] * w_out[k][i];
        delta[H_LAYERS-1][i] = dh * hidden[H_LAYERS-1][i] * (1 - hidden[H_LAYERS-1][i]);
    }
    /* earlier hidden layers */
    for(int l=H_LAYERS-2;l>=0;l--){
        for(int i=0;i<H_SZ;i++){
            double dh = 0.0;
            for(int j=0;j<H_SZ;j++) dh += delta[l+1][j] * w_hidden[l][j][i];
            delta[l][i] = dh * hidden[l][i] * (1 - hidden[l][i]);
        }
    }
    /* update hidden->hidden weights */
    for(int l=H_LAYERS-2;l>=0;l--){
        for(int i=0;i<H_SZ;i++){
            for(int j=0;j<H_SZ;j++){
                w_hidden[l][i][j] -= LR * delta[l+1][i] * hidden[l][j];
            }
            b_hidden[l+1][i] -= LR * delta[l+1][i];
        }
    }
    /* update input->first hidden weights */
    for(int i=0;i<H_SZ;i++){
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) w_input[i][j] -= LR * delta[0][i] * in_seq[j];
        b_hidden[0][i] -= LR * delta[0][i];
    }
}

/* legacy wrapper that pushes incoming single vector into internal buffer and, when enough
   history is present, trains to predict the newest vector from the previous SEQ_LEN vectors. */
void neuron_train_step(const double in[INPUT_N], double target /* unused as vector target is taken from sequence */){
    /* push into circular buffer */
    memcpy(mem_buf[mem_pos], in, sizeof(double)*INPUT_N);
    mem_pos = (mem_pos + 1) % SEQ_LEN;
    if(mem_cnt < SEQ_LEN) mem_cnt++;
    if(mem_cnt < SEQ_LEN) return; /* need more history */
    /* build sequence: order oldest..newest */
    double seq_in[INPUT_N * SEQ_LEN];
    int base = mem_pos; /* mem_pos points to next write; oldest is mem_pos */
    for(int s=0;s<SEQ_LEN;s++){
        int idx = (base + s) % SEQ_LEN;
        for(int k=0;k<INPUT_N;k++) seq_in[s*INPUT_N + k] = mem_buf[idx][k];
    }
    /* target vector is the last element in the sequence (we predict next vector; training here uses last as target) */
    double target_vec[INPUT_N];
    for(int k=0;k<INPUT_N;k++) target_vec[k] = seq_in[(SEQ_LEN-1)*INPUT_N + k];
    /* train to map previous SEQ_LEN vectors to the last one */
    neuron_train_step_seq(seq_in, target_vec);
}

/* legacy single-value forward: push input into buffer and return first component of predicted next vector (if available) */
double neuron_forward(const double in[INPUT_N], double hidden_out[H_SZ]){
    /* push into circular buffer (do not advance mem_pos here, keep consistent with train wrapper) */
    memcpy(mem_buf[mem_pos], in, sizeof(double)*INPUT_N);
    mem_pos = (mem_pos + 1) % SEQ_LEN;
    if(mem_cnt < SEQ_LEN) mem_cnt++;
    if(mem_cnt < SEQ_LEN) return 0.0;
    double seq_in[INPUT_N * SEQ_LEN];
    int base = mem_pos;
    for(int s=0;s<SEQ_LEN;s++){
        int idx = (base + s) % SEQ_LEN;
        for(int k=0;k<INPUT_N;k++) seq_in[s*INPUT_N + k] = mem_buf[idx][k];
    }
    double out[INPUT_N];
    neuron_predict_seq(seq_in, out);
    /* optional: fill hidden_out with first hidden layer activations if requested (best-effort) */
    if(hidden_out){
        /* not storing intermediate hidden in this wrapper; leave zeros */
        for(int i=0;i<H_SZ;i++) hidden_out[i] = 0.0;
    }
    return last_pred_scalar;
}

double neuron_get_last_prediction(void){ return last_pred_scalar; }

int neuron_save_weights(void){
    FILE *f = fopen(NN_WEIGHTS_FILE, "wb");
    if(!f) return -1;
    const char hdr[8] = "NNW2\0\0\0"; /* versioned header */
    if(fwrite(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { fclose(f); return -1; }
    /* save input weights */
    for(int i=0;i<H_SZ;i++){
        if(fwrite(w_input[i], sizeof(double), INPUT_N*SEQ_LEN, f) != INPUT_N*SEQ_LEN){ fclose(f); return -1; }
        if(fwrite(&b_hidden[0][i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    /* save hidden layers */
    for(int l=0;l<H_LAYERS-1;l++){
        for(int i=0;i<H_SZ;i++){
            if(fwrite(w_hidden[l][i], sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
            if(fwrite(&b_hidden[l+1][i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
        }
    }
    /* save output layer */
    for(int k=0;k<INPUT_N;k++){
        if(fwrite(w_out[k], sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
        if(fwrite(&b_out[k], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}

int neuron_load_weights(void){
    FILE *f = fopen(NN_WEIGHTS_FILE, "rb");
    if(!f) return -1;
    char hdr[8];
    if(fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)){ fclose(f); return -1; }
    if(strncmp(hdr, "NNW2", 4) != 0){ fclose(f); return -1; }
    for(int i=0;i<H_SZ;i++){
        if(fread(w_input[i], sizeof(double), INPUT_N*SEQ_LEN, f) != INPUT_N*SEQ_LEN){ fclose(f); return -1; }
        if(fread(&b_hidden[0][i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    for(int l=0;l<H_LAYERS-1;l++){
        for(int i=0;i<H_SZ;i++){
            if(fread(w_hidden[l][i], sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
            if(fread(&b_hidden[l+1][i], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
        }
    }
    for(int k=0;k<INPUT_N;k++){
        if(fread(w_out[k], sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
        if(fread(&b_out[k], sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}
