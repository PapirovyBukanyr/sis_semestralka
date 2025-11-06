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
