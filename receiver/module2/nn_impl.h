/**
 * nn.h
 * 
 * Declarations for the neural network interface used in module2.
 */

#ifndef NN_IMPL_H
#define NN_IMPL_H

#include "nn.h"

nn_t* nn_create(const nn_params_t *params);
void nn_free(nn_t* nn);

double nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw);

int nn_save_weights(nn_t* nn, const char* filename);
int nn_load_weights(nn_t* nn, const char* filename);

#endif
