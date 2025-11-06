#ifndef RECEIVER_MODULE2_NEURON_H
#define RECEIVER_MODULE2_NEURON_H



/* Ensure a default sequence length is available for nn.c and related code.
   Adjust SEQ_LEN to the intended model sequence length if different.
*/
#ifndef SEQ_LEN
#define SEQ_LEN 8
#endif

/* Make hidden-size macro names interchangeable to fix mismatches between
   declaration and implementation files that use H_SZ or HIDDEN_N.
*/
#ifdef H_SZ
  #ifndef HIDDEN_N
    #define HIDDEN_N H_SZ
  #endif
#endif

#ifdef HIDDEN_N
  #ifndef H_SZ
    #define H_SZ HIDDEN_N
  #endif
#endif

/* Ensure INPUT_N is defined elsewhere; keep existing dependency.
   Declare neuron_forward using H_SZ so implementations using H_SZ match.
*/
double neuron_forward(const double in[INPUT_N], double hidden_out[H_SZ]);

/* Sequence-based training / prediction prototypes referenced in nn.c */
void neuron_train_step_seq(const double *seq_in, const double *target_vec);
void neuron_predict_seq(const double *seq_in, double *pred_vec);

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
/* No-op placeholder (was previous content) */

/* Convenience helper: evaluate the network for two scalar inputs.
 * This constructs the input array and calls neuron_forward. Returns
 * the network output in [0,1]. Use this from tests or quick checks.
 */
static inline double neuron_eval_vals(double in0, double in1){
	double in[INPUT_N];
	in[0] = in0; in[1] = in1;
	return neuron_forward(in, NULL);
}

/* Ensure the neuron API sees shared project types/macros (INPUT_N, H_SZ, HIDDEN_N, SEQ_LEN, LR, etc.)
   by including the central types.h. Also include <stddef.h> so NULL is available.
   Do not redefine SEQ_LEN here to avoid conflicts with source files that set it.
*/
#include <stddef.h>
#include "../types.h"

#endif /* RECEIVER_MODULE2_NEURON_H */
