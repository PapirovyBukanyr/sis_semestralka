/*
 * nn_params.c
 *
 * Default neural-network parameter definitions and small helpers for
 * construction of parameter sets used by the network at runtime.
 */

#include "nn_params.h"

/**
 * Return a set of default neural network parameters.
 *
 * The returned structure contains sensible defaults for hidden layer count,
 * learning rate and feature scales derived from the dataset. Scales map raw
 * values to the network's normalized domain by dividing raw values by the
 * corresponding scale factor.
 *
 * @return initialized nn_params_t structure
 */
nn_params_t default_nn_params(){
    nn_params_t p;

    p.n_hidden_layers = 5; // default to 5 hidden layers

    p.neurons_per_layer = NULL; // use default layer sizes in nn_create if NULL

    p.learning_rate = 1e-2; // begin learning rate with 0.1, then decrease as needed 

    // scales derived from dataset statistics (max values observed in training data)
    p.scales[0] = 31075704.787;  
    p.scales[1] = 335.54333333;  
    p.scales[2] = 28642.12; 
    p.scales[3] = 28.47470817; 
    p.scales[4] = 865677.7584; 
    p.scales[5] = 4782337.7; 

    return p;
}
