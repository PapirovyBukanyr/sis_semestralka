#include "nn_params.h"

// Scales derived from provided dataset (max absolute values). These map raw->[-1,1] by dividing by scale.
// export_bytes, export_flows, export_packets, export_rtr, export_rtt, export_srt

nn_params_t default_nn_params(){
    nn_params_t p;
    p.n_hidden_layers = 0;
    p.neurons_per_layer = NULL; // caller will set or use provided
    // Increase default learning rate for quicker visible online updates during debugging.
    // Production tuning may want a much smaller value.
    p.learning_rate = 1e2; // was 1e-6
    // scales computed from uploaded dataset
    p.scales[0] = 31075704.787; // export_bytes
    p.scales[1] = 335.54333333; // export_flows
    p.scales[2] = 28642.12; // export_packets
    p.scales[3] = 28.47470817; // export_rtr
    p.scales[4] = 865677.7584; // export_rtt
    p.scales[5] = 4782337.7; // export_srt
    return p;
}
