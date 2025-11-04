#ifndef NEURON_H
#define NEURON_H

#include <stdint.h>

#define INPUT_N 2
#define HIDDEN_N 8
#define LR 0.001

/* Initialize weights randomly */
void neuron_rand_init(void);

/* Load/save weights (0 success, -1 failure) */
int neuron_load_weights(void);
int neuron_save_weights(void);

/* Perform a forward pass. 'hidden_out' may be NULL if not needed. Returns output in [0,1]. */
double neuron_forward(const double in[INPUT_N], double hidden_out[HIDDEN_N]);

/* Single training step update (in-place weight update) */
void neuron_train_step(const double in[INPUT_N], double target);

/* Last predicted output from the most recent forward/train call */
double neuron_get_last_prediction(void);

#endif /* NEURON_H */
