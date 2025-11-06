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
