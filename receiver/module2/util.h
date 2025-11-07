#ifndef MODULE2_UTIL_H
#define MODULE2_UTIL_H

#include "nn_params.h"
#include "../types.h"

void normalize_input(const nn_params_t* params, const data_point_t* in, double* out_norm);
void denormalize_output(const nn_params_t* params, const double* nn_out, float* out_raw);

#endif
