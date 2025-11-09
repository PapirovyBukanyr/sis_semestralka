/**
 * nn.h
 * 
 * Declarations for the neural network interface used in module2.
 */

#ifndef NN_PARAMS_H
#define NN_PARAMS_H

#include <stddef.h>
#define INPUT_SIZE 6
#define OUTPUT_SIZE 6

/**
 * Neural network parameters structure.
 * 
 * n_hidden_layers: number of hidden layers
 * neurons_per_layer: array of neuron counts per hidden layer (length n_hidden_layers)
 * learning_rate: learning rate for online training
 * scales: array of scale factors for input normalization (length INPUT_SIZE)
 */
typedef struct {
    size_t n_hidden_layers;
    size_t *neurons_per_layer; 
    double learning_rate;
    double scales[INPUT_SIZE]; 
} nn_params_t;

nn_params_t default_nn_params();

#endif
