#ifndef NEURON_LAYER_H
#define NEURON_LAYER_H

#include "neuron.h"
#include "nn_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If `nn_config.h` didn't already define these, provide safe defaults. */
#ifndef SEQ_LEN
#define SEQ_LEN 3
#endif
#ifndef H_LAYERS
#define H_LAYERS 10
#endif
#ifndef H_SZ
#define H_SZ 5
#endif

/* Sequence-based API: predict a vector from concatenated past SEQ_LEN vectors
   and perform a single training step on a sequence->target vector pair.
   NOTE: The output/target vector length was changed to INPUT_N * SEQ_LEN so
   the network can reconstruct the full recent sequence (autoencoder-like).
*/
void neuron_predict_seq(const double in_seq[INPUT_N * SEQ_LEN], double out[INPUT_N * SEQ_LEN]);
void neuron_train_step_seq(const double in_seq[INPUT_N * SEQ_LEN], const double target[INPUT_N * SEQ_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* NEURON_LAYER_H */
