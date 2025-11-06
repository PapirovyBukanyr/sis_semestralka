#ifndef RECEIVER_MODULE2_NEURON_H
#define RECEIVER_MODULE2_NEURON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Neural network (module2) public API.
 * We use unsized array parameters in the header so including files
 * don't need NN config macros to be visible at parse time.
 */
void neuron_rand_init(void);
int neuron_save_weights(void);
int neuron_load_weights(void);
void neuron_train_step(const double in[], double target);
void neuron_train_step_seq(const double in_seq[], const double target[]);
void neuron_predict_seq(const double in_seq[], double out[]);
double neuron_forward(const double in[], double hidden_out[]);
double neuron_get_last_prediction(void);

#ifdef __cplusplus
}
#endif

#endif // RECEIVER_MODULE2_NEURON_H
