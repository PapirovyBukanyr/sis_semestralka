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
/* reduce default depth to avoid vanishing gradients in small datasets */
#define H_LAYERS 2
#endif

#ifndef H_SZ
/* slightly larger hidden size for more capacity */
#define H_SZ 16
#endif

/* Learning rate for SGD updates */
#ifndef LR
/* smaller learning rate for stability with deeper nets */
#define LR 0.005
#endif

#endif /* RECEIVER_MODULE2_NN_CONFIG_H */
