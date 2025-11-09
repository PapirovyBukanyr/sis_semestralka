/**
 * util.c
 * 
 * Normalize input features to [-1, 1] range using precomputed scales and from [-1, 1] back to raw values.
 */

#include "util.h"

/** 
 * Normalize input features to [-1, 1] range using precomputed scales.
 * 
 * @param nn_params_t* params: pointer to nn_params_t containing scales
 * @param data_point_t* in: pointer to input data point (raw values)
 * @param double* out_norm: pointer to output array (length INPUT_SIZE) for normalized values
 */
void normalize_input(const nn_params_t* params, const data_point_t* in, double* out_norm){
    out_norm[0] = in->export_bytes / params->scales[0];
    out_norm[1] = in->export_flows / params->scales[1];
    out_norm[2] = in->export_packets / params->scales[2];
    out_norm[3] = in->export_rtr / params->scales[3];
    out_norm[4] = in->export_rtt / params->scales[4];
    out_norm[5] = in->export_srt / params->scales[5];
}

/** 
 * Denormalize neural network output from [-1, 1] range back to raw values using precomputed scales.
 *
 * @param nn_params_t* params: pointer to nn_params_t containing scales
 * @param double* nn_out: pointer to neural network output array (length OUTPUT_SIZE)
 * @param float* out_raw: pointer to output array (length OUTPUT_SIZE) for denormalized raw values
 */
void denormalize_output(const nn_params_t* params, const double* nn_out, float* out_raw){
    out_raw[0] = (float)(nn_out[0] * params->scales[0]);
    out_raw[1] = (float)(nn_out[1] * params->scales[1]);
    out_raw[2] = (float)(nn_out[2] * params->scales[2]);
    out_raw[3] = (float)(nn_out[3] * params->scales[3]);
    out_raw[4] = (float)(nn_out[4] * params->scales[4]);
    out_raw[5] = (float)(nn_out[5] * params->scales[5]);
}
