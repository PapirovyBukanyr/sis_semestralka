/**
 * neuron.h
 * 
 * Declarations for the neuron structure and functions used in module2.
 */

#ifndef NEURON_H
#define NEURON_H


#include <stddef.h>
#include <stdio.h>


typedef struct {
    size_t in_len; 
    double *w;
    double b;
} neuron_t;

neuron_t* neuron_create(size_t in_len);
void neuron_free(neuron_t* n);
double neuron_forward(const neuron_t* n, const double* input);
void neuron_update(neuron_t* n, const double* input, double grad_out, double lr);
int neuron_write(FILE* f, const neuron_t* n);
int neuron_read(FILE* f, neuron_t* n);

#endif
