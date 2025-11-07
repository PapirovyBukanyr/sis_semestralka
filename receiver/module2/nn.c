/* === nn.h === */
#ifndef NN_H
#define NN_H

#include <stddef.h>
#include "nn_params.h"
#include "../types.h"


typedef struct nn_s nn_t;

nn_t* nn_create(const nn_params_t *params);
void nn_free(nn_t* nn);

// Provide input as data_point_t (raw), returns prediction in out vector (length = OUTPUT_SIZE)
// If target != NULL, it's a pointer to target output (raw values) and online training occurs.
void nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw);

// load/save weights
int nn_save_weights(nn_t* nn, const char* filename);
int nn_load_weights(nn_t* nn, const char* filename);

#endif

/* === neuron.h === */
#ifndef NEURON_H
#define NEURON_H

#include <stddef.h>

typedef struct {
    size_t in_len; // number of inputs
    double *w;     // weights length in_len
    double b;      // bias (C in spec)
} neuron_t;

neuron_t* neuron_create(size_t in_len);
void neuron_free(neuron_t* n);

// compute V = A*B + C (dot product + bias)
double neuron_forward(const neuron_t* n, const double* input);

// gradient update: lr, gradient w.r.t output (dL/dV)
void neuron_update(neuron_t* n, const double* input, double grad_out, double lr);

// io
int neuron_write(FILE* f, const neuron_t* n);
int neuron_read(FILE* f, neuron_t* n);

#endif

/* === neuron.c === */
#include "neuron.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

static double drand_unit(){ return (double)rand() / (double)RAND_MAX * 2.0 - 1.0; }

neuron_t* neuron_create(size_t in_len){
    neuron_t* n = (neuron_t*)calloc(1,sizeof(neuron_t));
    if(!n) return NULL;
    n->in_len = in_len;
    n->w = (double*)malloc(sizeof(double)*in_len);
    if(!n->w){ free(n); return NULL; }
    // init small random weights
    for(size_t i=0;i<in_len;i++) n->w[i] = drand_unit()*0.1;
    n->b = drand_unit()*0.1;
    return n;
}

void neuron_free(neuron_t* n){ if(!n) return; free(n->w); free(n); }

double neuron_forward(const neuron_t* n, const double* input){
    double s = 0.0;
    for(size_t i=0;i<n->in_len;i++) s += n->w[i] * input[i];
    s += n->b;
    return s; // linear as requested
}

void neuron_update(neuron_t* n, const double* input, double grad_out, double lr){
    // grad_out is dL/dV (scalar)
    for(size_t i=0;i<n->in_len;i++){
        double g = grad_out * input[i];
        n->w[i] -= lr * g;
    }
    n->b -= lr * grad_out;
}

int neuron_write(FILE* f, const neuron_t* n){
    if(fwrite(&n->in_len,sizeof(size_t),1,f)!=1) return -1;
    if(fwrite(n->w,sizeof(double),n->in_len,f)!=n->in_len) return -1;
    if(fwrite(&n->b,sizeof(double),1,f)!=1) return -1;
    return 0;
}

int neuron_read(FILE* f, neuron_t* n){
    size_t in_len=0;
    if(fread(&in_len,sizeof(size_t),1,f)!=1) return -1;
    if(in_len != n->in_len) return -1; // mismatch
    if(fread(n->w,sizeof(double),n->in_len,f)!=n->in_len) return -1;
    if(fread(&n->b,sizeof(double),1,f)!=1) return -1;
    return 0;
}

/* === h_layer.h === */
#ifndef H_LAYER_H
#define H_LAYER_H

#include <stddef.h>
#include "neuron.h"

typedef struct {
    size_t n_neurons;
    neuron_t **neurons; // array of pointers
    size_t input_len; // inputs per neuron
} h_layer_t;

h_layer_t* h_layer_create(size_t n_neurons, size_t input_len);
void h_layer_free(h_layer_t* L);

// forward: input is length input_len, output is preallocated array length n_neurons
void h_layer_forward(const h_layer_t* L, const double* input, double* output);

int h_layer_write(FILE* f, const h_layer_t* L);
int h_layer_read(FILE* f, h_layer_t* L);

#endif

/* === h_layer.c === */
#include "h_layer.h"
#include <stdlib.h>
#include <stdio.h>

h_layer_t* h_layer_create(size_t n_neurons, size_t input_len){
    h_layer_t* L = (h_layer_t*)calloc(1,sizeof(h_layer_t));
    if(!L) return NULL;
    L->n_neurons = n_neurons;
    L->input_len = input_len;
    L->neurons = (neuron_t**)malloc(sizeof(neuron_t*)*n_neurons);
    if(!L->neurons){ free(L); return NULL; }
    for(size_t i=0;i<n_neurons;i++) L->neurons[i] = neuron_create(input_len);
    return L;
}

void h_layer_free(h_layer_t* L){ if(!L) return; for(size_t i=0;i<L->n_neurons;i++) neuron_free(L->neurons[i]); free(L->neurons); free(L); }

void h_layer_forward(const h_layer_t* L, const double* input, double* output){
    for(size_t i=0;i<L->n_neurons;i++) output[i] = neuron_forward(L->neurons[i], input);
}

int h_layer_write(FILE* f, const h_layer_t* L){
    if(fwrite(&L->n_neurons,sizeof(size_t),1,f)!=1) return -1;
    if(fwrite(&L->input_len,sizeof(size_t),1,f)!=1) return -1;
    for(size_t i=0;i<L->n_neurons;i++) if(neuron_write(f,L->neurons[i])!=0) return -1;
    return 0;
}

int h_layer_read(FILE* f, h_layer_t* L){
    size_t n_neurons=0, input_len=0;
    if(fread(&n_neurons,sizeof(size_t),1,f)!=1) return -1;
    if(fread(&input_len,sizeof(size_t),1,f)!=1) return -1;
    if(n_neurons != L->n_neurons || input_len != L->input_len) return -1;
    for(size_t i=0;i<L->n_neurons;i++) if(neuron_read(f,L->neurons[i])!=0) return -1;
    return 0;
}

/* === nn_params.h === */
#ifndef NN_PARAMS_H
#define NN_PARAMS_H

#include <stddef.h>
#define INPUT_SIZE 6
#define OUTPUT_SIZE 6

typedef struct {
    size_t n_hidden_layers;
    size_t *neurons_per_layer; // length n_hidden_layers
    double learning_rate;
    double scales[INPUT_SIZE]; // scale factors for normalization (divide by scale)
} nn_params_t;

nn_params_t default_nn_params();

#endif

/* === nn_params.c === */
#include "nn_params.h"

// Scales derived from provided dataset (max absolute values). These map raw->[-1,1] by dividing by scale.
// export_bytes, export_flows, export_packets, export_rtr, export_rtt, export_srt

nn_params_t default_nn_params(){
    nn_params_t p;
    p.n_hidden_layers = 2;
    p.neurons_per_layer = NULL; // caller will set or use provided
    p.learning_rate = 1e-6; // small, tune as needed
    // scales computed from uploaded dataset
    p.scales[0] = 31075704.787; // export_bytes
    p.scales[1] = 335.54333333; // export_flows
    p.scales[2] = 28642.12; // export_packets
    p.scales[3] = 28.47470817; // export_rtr
    p.scales[4] = 865677.7584; // export_rtt
    p.scales[5] = 4782337.7; // export_srt
    return p;
}

/* util functions are provided in separate util.h / util.c to be shared by nn_impl.c */
#include "util.h"

// The combined-source file was split into separate files.
// See the individual headers and implementation files in this directory:
// - nn.h
// - neuron.h
// - neuron.c
// - h_layer.h
// - h_layer.c
// - nn_params.h
// - nn_params.c
// - nn_impl.c  (nn implementation)
// NOTE: util.h/c and main.c and the Makefile blocks were intentionally left in the combined source and are
// not extracted by this automatic split operation per user request.


/* === main.c === */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nn.h"
#include "nn_params.h"
#include "util.h"

int main(){
    nn_params_t params = default_nn_params();
    size_t neurons_per_layer[] = {32,16};
    params.n_hidden_layers = 2;
    params.neurons_per_layer = NULL; // library will use defaults unless you allocate

    nn_t* nn = nn_create(&params);
    if(!nn){ fprintf(stderr,"Failed to create network\n"); return 1; }
    // try load existing weights, if not present it's fine
    if(nn_load_weights(nn, "nn_weights.bin")!=0) fprintf(stderr, "No weights loaded, starting from random init\n");

    // Example usage: create a dummy data_point and run prediction
    data_point_t dp = {0};
    dp.export_bytes = 30000.0f;
    dp.export_flows = 10.0f;
    dp.export_packets = 1200.0f;
    dp.export_rtr = 5.0f;
    dp.export_rtt = 10000.0f;
    dp.export_srt = 20000.0f;

    float pred[OUTPUT_SIZE];
    nn_predict_and_maybe_train(nn, &dp, NULL, pred);
    printf("Predicted (raw) outputs:\n");
    for(size_t i=0;i<OUTPUT_SIZE;i++) printf(" %f", pred[i]);
    printf("\n");

    // cleanup
    nn_free(nn);
    return 0;
}
