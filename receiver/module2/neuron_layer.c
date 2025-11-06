#include "neuron.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Persistence filenames (placed in data/ to keep repo layout) */
#define NN_WEIGHTS_FILE "data/nn_weights.bin"

/* Sequence length (number of past vectors used) */
#define SEQ_LEN 3
/* Hidden layers count and size */
#define H_LAYERS 10
#define H_SZ 5

static double last_pred_scalar = 0.0; /* legacy single-value API: store first component */
static double last_pred_vec[INPUT_N];

/* Structured neuron representation */
typedef struct { double w[INPUT_N * SEQ_LEN]; double b; } InputNeuron; /* first hidden layer neurons */
typedef struct { double w[H_SZ]; double b; } HiddenNeuron; /* hidden-layer neuron (weights from previous hidden layer) */
typedef struct { double w[H_SZ]; double b; } OutNeuron; /* one output neuron per input-dim */

static InputNeuron input_layer[H_SZ];
static HiddenNeuron hidden_layers[H_LAYERS-1][H_SZ]; /* transitions between hidden layers */
static HiddenNeuron hidden_first_bias[1][H_SZ]; /* biases for first hidden layer are in input_layer.b but keep layout compatibility */
static OutNeuron output_layer[INPUT_N];

/* internal sequence memory (legacy wrapper compatibility) */
static double mem_buf[SEQ_LEN][INPUT_N];
static int mem_cnt = 0;
static int mem_pos = 0;

static double sigmoid(double x){ return 1.0 / (1.0 + exp(-x)); }

void neuron_rand_init(void){
    for(int i=0;i<H_SZ;i++){
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) input_layer[i].w[j] = ((double)rand()/RAND_MAX - 0.5)*0.1;
        input_layer[i].b = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
    for(int l=0;l<H_LAYERS-1;l++){
        for(int i=0;i<H_SZ;i++){
            for(int j=0;j<H_SZ;j++) hidden_layers[l][i].w[j] = ((double)rand()/RAND_MAX - 0.5)*0.1;
            hidden_layers[l][i].b = ((double)rand()/RAND_MAX - 0.5)*0.1;
        }
    }
    for(int k=0;k<INPUT_N;k++){
        for(int i=0;i<H_SZ;i++) output_layer[k].w[i] = ((double)rand()/RAND_MAX - 0.5)*0.1;
        output_layer[k].b = ((double)rand()/RAND_MAX - 0.5)*0.1;
    }
}

void neuron_predict_seq(const double in_seq[INPUT_N * SEQ_LEN], double out[INPUT_N]){
    double hidden[H_LAYERS][H_SZ];
    /* first hidden layer using input_layer neurons */
    for(int i=0;i<H_SZ;i++){
        double s = input_layer[i].b;
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) s += input_layer[i].w[j] * in_seq[j];
        hidden[0][i] = sigmoid(s);
    }
    /* propagate through hidden layers */
    for(int l=1;l<H_LAYERS;l++){
        for(int i=0;i<H_SZ;i++){
            double s = (l==0?0.0:hidden_layers[l-1][i].b); /* not used for l==0 */
            /* compute dot of weights of neuron i in layer l-1 with previous hidden activations
               note: hidden_layers indexed as [layer-1][neuron_i].w[j] contains weight from prev neuron j to neuron i */
            for(int j=0;j<H_SZ;j++) s += hidden_layers[l-1][i].w[j] * hidden[l-1][j];
            hidden[l][i] = sigmoid(s);
        }
    }
    /* output layer (vector) */
    for(int k=0;k<INPUT_N;k++){
        double s = output_layer[k].b;
        for(int i=0;i<H_SZ;i++) s += output_layer[k].w[i] * hidden[H_LAYERS-1][i];
        out[k] = sigmoid(s);
        last_pred_vec[k] = out[k];
    }
    last_pred_scalar = last_pred_vec[0];
}

void neuron_train_step_seq(const double in_seq[INPUT_N * SEQ_LEN], const double target[INPUT_N]){
    double hidden[H_LAYERS][H_SZ];
    /* forward pass (same as predict but store hidden) */
    for(int i=0;i<H_SZ;i++){
        double s = input_layer[i].b;
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) s += input_layer[i].w[j] * in_seq[j];
        hidden[0][i] = sigmoid(s);
    }
    for(int l=1;l<H_LAYERS;l++){
        for(int i=0;i<H_SZ;i++){
            double s = hidden_layers[l-1][i].b;
            for(int j=0;j<H_SZ;j++) s += hidden_layers[l-1][i].w[j] * hidden[l-1][j];
            hidden[l][i] = sigmoid(s);
        }
    }
    double out[INPUT_N];
    for(int k=0;k<INPUT_N;k++){
        double s = output_layer[k].b;
        for(int i=0;i<H_SZ;i++) s += output_layer[k].w[i] * hidden[H_LAYERS-1][i];
        out[k] = sigmoid(s);
    }
    /* compute output gradients and update output weights */
    double dout[INPUT_N];
    for(int k=0;k<INPUT_N;k++) dout[k] = (out[k] - target[k]) * out[k] * (1 - out[k]);
    for(int k=0;k<INPUT_N;k++){
        for(int i=0;i<H_SZ;i++) output_layer[k].w[i] -= LR * dout[k] * hidden[H_LAYERS-1][i];
        output_layer[k].b -= LR * dout[k];
    }
    /* backprop into hidden layers */
    double delta[H_LAYERS][H_SZ];
    for(int i=0;i<H_SZ;i++){
        double dh = 0.0;
        for(int k=0;k<INPUT_N;k++) dh += dout[k] * output_layer[k].w[i];
        delta[H_LAYERS-1][i] = dh * hidden[H_LAYERS-1][i] * (1 - hidden[H_LAYERS-1][i]);
    }
    for(int l=H_LAYERS-2;l>=0;l--){
        for(int i=0;i<H_SZ;i++){
            double dh = 0.0;
            for(int j=0;j<H_SZ;j++) dh += delta[l+1][j] * hidden_layers[l][j].w[i];
            delta[l][i] = dh * hidden[l][i] * (1 - hidden[l][i]);
        }
    }
    /* update hidden->hidden weights and biases */
    for(int l=H_LAYERS-2;l>=0;l--){
        for(int i=0;i<H_SZ;i++){
            for(int j=0;j<H_SZ;j++){
                hidden_layers[l][i].w[j] -= LR * delta[l+1][i] * hidden[l][j];
            }
            hidden_layers[l][i].b -= LR * delta[l+1][i];
        }
    }
    /* update input->first hidden weights and biases */
    for(int i=0;i<H_SZ;i++){
        for(int j=0;j<INPUT_N*SEQ_LEN;j++) input_layer[i].w[j] -= LR * delta[0][i] * in_seq[j];
        input_layer[i].b -= LR * delta[0][i];
    }
}

void neuron_train_step(const double in[INPUT_N], double target /* unused */){
    memcpy(mem_buf[mem_pos], in, sizeof(double)*INPUT_N);
    mem_pos = (mem_pos + 1) % SEQ_LEN;
    if(mem_cnt < SEQ_LEN) mem_cnt++;
    if(mem_cnt < SEQ_LEN) return;
    double seq_in[INPUT_N * SEQ_LEN];
    int base = mem_pos;
    for(int s=0;s<SEQ_LEN;s++){
        int idx = (base + s) % SEQ_LEN;
        for(int k=0;k<INPUT_N;k++) seq_in[s*INPUT_N + k] = mem_buf[idx][k];
    }
    double target_vec[INPUT_N];
    for(int k=0;k<INPUT_N;k++) target_vec[k] = seq_in[(SEQ_LEN-1)*INPUT_N + k];
    neuron_train_step_seq(seq_in, target_vec);
}

double neuron_forward(const double in[INPUT_N], double hidden_out[H_SZ]){
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
    if(hidden_out){
        for(int i=0;i<H_SZ;i++) hidden_out[i] = 0.0;
    }
    return last_pred_scalar;
}

double neuron_get_last_prediction(void){ return last_pred_scalar; }

int neuron_save_weights(void){
    FILE *f = fopen(NN_WEIGHTS_FILE, "wb");
    if(!f) return -1;
    const char hdr[8] = "NNW2\0\0\0";
    if(fwrite(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { fclose(f); return -1; }
    /* save input layer */
    for(int i=0;i<H_SZ;i++){
        if(fwrite(input_layer[i].w, sizeof(double), INPUT_N*SEQ_LEN, f) != INPUT_N*SEQ_LEN){ fclose(f); return -1; }
        if(fwrite(&input_layer[i].b, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    /* save hidden layers */
    for(int l=0;l<H_LAYERS-1;l++){
        for(int i=0;i<H_SZ;i++){
            if(fwrite(hidden_layers[l][i].w, sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
            if(fwrite(&hidden_layers[l][i].b, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
        }
    }
    /* save output layer */
    for(int k=0;k<INPUT_N;k++){
        if(fwrite(output_layer[k].w, sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
        if(fwrite(&output_layer[k].b, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
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
        if(fread(input_layer[i].w, sizeof(double), INPUT_N*SEQ_LEN, f) != INPUT_N*SEQ_LEN){ fclose(f); return -1; }
        if(fread(&input_layer[i].b, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    for(int l=0;l<H_LAYERS-1;l++){
        for(int i=0;i<H_SZ;i++){
            if(fread(hidden_layers[l][i].w, sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
            if(fread(&hidden_layers[l][i].b, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
        }
    }
    for(int k=0;k<INPUT_N;k++){
        if(fread(output_layer[k].w, sizeof(double), H_SZ, f) != H_SZ){ fclose(f); return -1; }
        if(fread(&output_layer[k].b, sizeof(double), 1, f) != 1){ fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}
