/* === nn.h === */
#ifndef NN_H
#define NN_H



#include <stddef.h>
#include "nn_params.h"
/* Use the shared data_point_t from receiver/types.h to avoid duplicate definitions */
#include "../types.h"

typedef struct nn_s nn_t;


nn_t* nn_create(const nn_params_t *params);
void nn_free(nn_t* nn);


// Provide input as data_point_t (raw), returns prediction in out vector (length = OUTPUT_SIZE)
// If target != NULL, it's a pointer to target output (raw values) and online training occurs.
// Returns Euclidean cost after training if target provided, otherwise returns NAN.
double nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw);

/* Thread entrypoint used by the receiver pipeline (created from io.c) */
void* nn_thread(void* arg);


// load/save weights
int nn_save_weights(nn_t* nn, const char* filename);
int nn_load_weights(nn_t* nn, const char* filename);


#endif
