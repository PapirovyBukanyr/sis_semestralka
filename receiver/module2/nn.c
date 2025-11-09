/*
 * nn.c
 *
 * Neural network top-level implementation: wiring layers, prediction and
 * coordination helpers. Provides the runtime functions used by the
 * neural-network module.
 */

#include <stddef.h>

#include "nn_params.h"
#include "../types.h"

typedef struct nn_s nn_t;

nn_t* nn_create(const nn_params_t *params);
void nn_free(nn_t* nn);
double nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw);
int nn_save_weights(nn_t* nn, const char* filename);
int nn_load_weights(nn_t* nn, const char* filename);

