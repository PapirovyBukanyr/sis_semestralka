#include "nn_params.h"

/**
 * Return a set of default neural network parameters.
 *
 * The returned structure contains sensible defaults for hidden layer count,
 * learning rate and feature scales derived from the dataset. Scales map raw
 * values to the network's normalized domain by dividing raw values by the
 * corresponding scale factor.
 *
 * @return initialized nn_params_t structure
 */
nn_params_t default_nn_params(){
    nn_params_t p;
    p.n_hidden_layers = 4;
    p.neurons_per_layer = NULL; /* caller will set or use provided */
    /* Slightly increased default learning rate to make online updates visible during debugging. */
    p.learning_rate = 1e-1;
    /* scales computed from uploaded dataset */
    p.scales[0] = 31075704.787; /* export_bytes */
    p.scales[1] = 335.54333333; /* export_flows */
    p.scales[2] = 28642.12; /* export_packets */
    p.scales[3] = 28.47470817; /* export_rtr */
    p.scales[4] = 865677.7584; /* export_rtt */
    p.scales[5] = 4782337.7; /* export_srt */
    return p;
}
