/** 
 * h_layer.h
 * 
 * Declarations for the hidden layer structure and functions used in module2.
 */

#ifndef H_LAYER_H
#define H_LAYER_H

#include <stddef.h>
#include <stdio.h>

#include "neuron.h"

typedef struct {
    size_t n_neurons;
    neuron_t **neurons; 
    size_t input_len;
} h_layer_t;
h_layer_t* h_layer_create(size_t n_neurons, size_t input_len);
void h_layer_free(h_layer_t* L);
void h_layer_forward(const h_layer_t* L, const double* input, double* output);
int h_layer_write(FILE* f, const h_layer_t* L);
int h_layer_read(FILE* f, h_layer_t* L);

#endif
