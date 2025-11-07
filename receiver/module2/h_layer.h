/* === h_layer.h === */
#ifndef H_LAYER_H
#define H_LAYER_H

#include <stddef.h>
#include <stdio.h>
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
