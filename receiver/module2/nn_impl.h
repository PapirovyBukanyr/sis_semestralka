/* === nn_impl.h === */
#ifndef NN_IMPL_H
#define NN_IMPL_H

#include "nn.h"

// Implementations for the neural network (defined in nn_impl.c)
nn_t* nn_create(const nn_params_t *params);
void nn_free(nn_t* nn);

// Provide input as data_point_t (raw), returns prediction in out vector (length = OUTPUT_SIZE)
// If target_raw != NULL, perform online training.
// Returns Euclidean cost after training if target provided, otherwise returns NAN.
double nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw);

// load/save weights
int nn_save_weights(nn_t* nn, const char* filename);
int nn_load_weights(nn_t* nn, const char* filename);

#endif
