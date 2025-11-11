/**
 * neuron.h
 * 
 * Declarations for the neuron structure and functions used in module2.
 */

#ifndef NEURON_H
#define NEURON_H


#include <stddef.h>
#include <stdio.h>


/** Supported activation functions for a neuron. */
typedef enum {
    ACT_LINEAR = 0,
    ACT_RELU,
    ACT_SIGMOID,
    ACT_TANH
} act_t;

/**
 * neuron_t
 * - in_len: number of inputs
 * - w: weight array of length in_len
 * - b: bias
 * - act: activation function used by the neuron
 * - last_z: last pre-activation value (z = w·x + b), stored for gradient computation
 */
typedef struct {
    size_t in_len;
    double *w;
    double b;
    act_t act;
    double last_z;
} neuron_t;

/* Create and manage neuron */
neuron_t* neuron_create(size_t in_len);
void neuron_free(neuron_t* n);

double neuron_forward(neuron_t* n, const double* input);

void neuron_update(neuron_t* n, const double* input, double grad_out, double lr);

int neuron_write(FILE* f, const neuron_t* n);
int neuron_read(FILE* f, neuron_t* n);

#endif
