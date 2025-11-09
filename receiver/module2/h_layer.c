/**
 * h_layer.c
 *
 * Implementation of the hidden-layer container used by the neural-network
 * module. Provides creation, destruction, forward pass and
 * serialization/deserialization helpers.
 */

#include <stdlib.h>
#include <stdio.h>

#include "h_layer.h"

/**
 * Create a new hidden layer with specified number of neurons and input length.
 *
 * @param n_neurons number of neurons in the layer
 * @param input_len number of inputs per neuron
 *
 * @return pointer to allocated h_layer_t or NULL on error
 */
h_layer_t* h_layer_create(size_t n_neurons, size_t input_len)
{
    h_layer_t* L = (h_layer_t*)calloc(1, sizeof(h_layer_t));
    if (!L) return NULL;
    L->n_neurons = n_neurons;
    L->input_len = input_len;
    L->neurons = (neuron_t**)malloc(sizeof(neuron_t*) * n_neurons);
    if (!L->neurons) { free(L); return NULL; }
    for (size_t i = 0; i < n_neurons; i++)
        L->neurons[i] = neuron_create(input_len);
    return L;
}

/**
 * Free a hidden layer and all its neurons.
 *
 * @param L pointer previously returned by h_layer_create (may be NULL)
 */
void h_layer_free(h_layer_t* L)
{
    if (!L) return;
    for (size_t i = 0; i < L->n_neurons; i++) {
        neuron_free(L->neurons[i]);
    }
    free(L->neurons);
    free(L);
}

/**
 * Forward pass through the hidden layer.
 *
 * @param L pointer to hidden layer
 * @param input input array of length L->input_len
 * @param output preallocated output array of length L->n_neurons
 */
void h_layer_forward(const h_layer_t* L, const double* input, double* output)
{
    for (size_t i = 0; i < L->n_neurons; i++) {
        output[i] = neuron_forward(L->neurons[i], input);
    }
}

/**
 * Write a hidden layer to a file.
 *
 * @param f open FILE* for writing
 * @param L pointer to hidden layer
 * @return 0 on success, -1 on error
 */
int h_layer_write(FILE* f, const h_layer_t* L)
{
    if (fwrite(&L->n_neurons, sizeof(size_t), 1, f) != 1) return -1;
    if (fwrite(&L->input_len, sizeof(size_t), 1, f) != 1) return -1;
    for (size_t i = 0; i < L->n_neurons; i++) {
        if (neuron_write(f, L->neurons[i]) != 0) return -1;
    }
    return 0;
}

/**
 * Read a hidden layer from a file.
 *
 * @param f open FILE* for reading
 * @param L pointer to hidden layer (must be preallocated with matching sizes)
 * @return 0 on success, -1 on error
 */
int h_layer_read(FILE* f, h_layer_t* L)
{
    size_t n_neurons = 0, input_len = 0;
    if (fread(&n_neurons, sizeof(size_t), 1, f) != 1) return -1;
    if (fread(&input_len, sizeof(size_t), 1, f) != 1) return -1;
    if (n_neurons != L->n_neurons || input_len != L->input_len) return -1;
    for (size_t i = 0; i < L->n_neurons; i++) {
        if (neuron_read(f, L->neurons[i]) != 0) return -1;
    }
    return 0;
}
