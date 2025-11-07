/* === neuron.h === */
#ifndef NEURON_H
#define NEURON_H


#include <stddef.h>
#include <stdio.h>


typedef struct {
size_t in_len; // number of inputs
double *w; // weights length in_len
double b; // bias (C in spec)
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
