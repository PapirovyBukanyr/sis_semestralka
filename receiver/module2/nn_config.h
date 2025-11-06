#ifndef RECEIVER_MODULE2_NN_CONFIG_H
#define RECEIVER_MODULE2_NN_CONFIG_H

/* Centralized neural network configuration for module2
   Adjust these macros to change the network architecture or training rate.
*/

/* Input vector dimension (number of scalar input features) */
#ifndef INPUT_N
#define INPUT_N 2
#endif

/* Sequence length (number of past vectors concatenated as input) */
#ifndef SEQ_LEN
#define SEQ_LEN 3
#endif

/* Hidden layer configuration */
#ifndef H_LAYERS
#define H_LAYERS 10
#endif

#ifndef H_SZ
#define H_SZ 5
#endif

/* Learning rate for SGD updates */
#ifndef LR
#define LR 0.01
#endif

#endif /* RECEIVER_MODULE2_NN_CONFIG_H */
