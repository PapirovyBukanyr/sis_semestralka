#include "nn.h"
#include "neuron.h"
#include "h_layer.h"
#include "nn_params.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../log.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

struct nn_s{
    nn_params_t params;
    size_t *neurons_per_layer;
    size_t n_layers; // hidden layers count
    h_layer_t **layers;
    // output layer as a simple linear layer of OUTPUT_SIZE neurons
    h_layer_t *output_layer;
};

nn_t* nn_create(const nn_params_t *p_in){
    nn_t* nn = (nn_t*)calloc(1,sizeof(nn_t));
    if(!nn) return NULL;
    nn->params = *p_in;
    // default neurons per layer if not provided
    size_t default_neurons[] = {32, 16};
    if(p_in->n_hidden_layers==0){
        nn->n_layers = 0;
        nn->neurons_per_layer = NULL;
    } else {
        nn->n_layers = p_in->n_hidden_layers;
        nn->neurons_per_layer = (size_t*)malloc(sizeof(size_t)*nn->n_layers);
        if(p_in->neurons_per_layer!=NULL){
            for(size_t i=0;i<nn->n_layers;i++) nn->neurons_per_layer[i] = p_in->neurons_per_layer[i];
        } else {
            for(size_t i=0;i<nn->n_layers;i++) nn->neurons_per_layer[i] = default_neurons[i%2];
        }
    }
    // create layers
    size_t prev_size = INPUT_SIZE;
    if(nn->n_layers>0){
        nn->layers = (h_layer_t**)malloc(sizeof(h_layer_t*)*nn->n_layers);
        for(size_t i=0;i<nn->n_layers;i++){
            nn->layers[i] = h_layer_create(nn->neurons_per_layer[i], prev_size);
            prev_size = nn->neurons_per_layer[i];
        }
    }
    // output layer
    nn->output_layer = h_layer_create(OUTPUT_SIZE, prev_size);
    /* seed RNG early (before neuron_create in layers) so initial weights are randomized */
    srand((unsigned)time(NULL));
    // try to load saved weights from disk; ignore errors (fresh start if missing/mismatch)
    if(nn_load_weights(nn, "data/nn_weights.bin")==0){
        LOG_INFO("[nn] loaded weights from data/nn_weights.bin\n");
    } else {
        LOG_INFO("[nn] no weight file loaded (starting with random weights)\n");
    }
    return nn;
}

void nn_free(nn_t* nn){
    if(!nn) return;
    // try to save weights; best-effort
    if(nn_save_weights(nn, "data/nn_weights.bin")==0){
        LOG_INFO("[nn] saved weights to data/nn_weights.bin\n");
    } else {
        LOG_ERROR("[nn] failed to save weights to data/nn_weights.bin\n");
    }
    if(nn->neurons_per_layer) free(nn->neurons_per_layer);
    if(nn->layers){ for(size_t i=0;i<nn->n_layers;i++) h_layer_free(nn->layers[i]); free(nn->layers); }
    if(nn->output_layer) h_layer_free(nn->output_layer);
    free(nn);
}

// internal forward producing normalized outputs (double array length OUTPUT_SIZE)
static void nn_forward(nn_t* nn, const double* input_norm, double* out_norm){
    double *cur = (double*)malloc(sizeof(double)* (size_t) 1024);
    double *tmp_in = (double*)malloc(sizeof(double)* 1024);
    // We allocate dynamically to avoid fixed max; but keep simple: allocate max 1024
    // copy input
    size_t cur_len = INPUT_SIZE;
    for(size_t i=0;i<cur_len;i++) cur[i] = input_norm[i];
    for(size_t L=0; L<nn->n_layers; L++){
        size_t n_neu = nn->layers[L]->n_neurons;
        // compute layer output into tmp_in
        for(size_t j=0;j<n_neu;j++) tmp_in[j] = neuron_forward(nn->layers[L]->neurons[j], cur);
        // swap
        cur_len = n_neu;
        for(size_t j=0;j<cur_len;j++) cur[j]=tmp_in[j];
    }
    // output layer
    for(size_t j=0;j<nn->output_layer->n_neurons;j++) out_norm[j] = neuron_forward(nn->output_layer->neurons[j], cur);
    free(cur); free(tmp_in);
}

double nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw){
    double input_norm[INPUT_SIZE];
    normalize_input(&nn->params, in, input_norm);

    /* Build layer size array: input, hidden..., output */
    size_t n_hidden = nn->n_layers;
    size_t n_layers_total = n_hidden + 2; /* input + hidden(s) + output */
    size_t *sizes = (size_t*)malloc(sizeof(size_t)*n_layers_total);
    if(!sizes) return NAN;
    sizes[0] = INPUT_SIZE;
    for(size_t i=0;i<n_hidden;i++) sizes[i+1] = nn->layers[i]->n_neurons;
    sizes[n_layers_total-1] = OUTPUT_SIZE;

    /* compute offsets and total neurons */
    size_t *offset = (size_t*)malloc(sizeof(size_t)*n_layers_total);
    if(!offset){ free(sizes); return NAN; }
    size_t acc = 0; for(size_t i=0;i<n_layers_total;i++){ offset[i]=acc; acc += sizes[i]; }
    size_t total_neurons = acc;

    double *acts = (double*)malloc(sizeof(double)*total_neurons);
    if(!acts){ free(sizes); free(offset); return NAN; }

    /* copy normalized input into activations */
    for(size_t i=0;i<INPUT_SIZE;i++) acts[offset[0]+i] = input_norm[i];

    /* forward pass through hidden layers and output */
    for(size_t L=1; L<n_layers_total; L++){
        double *prev_ptr = &acts[offset[L-1]];
        size_t cur_n = sizes[L];
        if(L <= n_hidden){
            h_layer_t *hl = nn->layers[L-1];
            for(size_t j=0;j<cur_n;j++) acts[offset[L]+j] = neuron_forward(hl->neurons[j], prev_ptr);
        } else {
            h_layer_t *ol = nn->output_layer;
            for(size_t j=0;j<cur_n;j++) acts[offset[L]+j] = neuron_forward(ol->neurons[j], prev_ptr);
        }
    }

    double *out_norm = &acts[offset[n_layers_total-1]];

    if(target_raw){
        /* normalize target into same domain as outputs */
        double target_norm[OUTPUT_SIZE];
        for(size_t i=0;i<OUTPUT_SIZE;i++) target_norm[i] = ((double)target_raw[i]) / nn->params.scales[i];

        /* deltas for all non-input neurons stored in flat array aligned with offset - sizes[0] */
        size_t deltas_len = total_neurons - sizes[0];
    double *deltas = (double*)malloc(sizeof(double)*deltas_len);
    if(!deltas){ free(acts); free(sizes); free(offset); return NAN; }

        /* compute output deltas and cost (MSE/EUCLIDEAN) */
        size_t out_layer_index = n_layers_total - 1;
        size_t out_off = offset[out_layer_index] - sizes[0];
        double sum_sq = 0.0;
        for(size_t j=0;j<sizes[out_layer_index]; j++){
            double y = out_norm[j];
            double t = target_norm[j];
            double diff = y - t;
            deltas[out_off + j] = diff; /* dL/dy for 0.5*sum(diff^2) */
            sum_sq += diff*diff;
        }

        /* backpropagate through hidden layers (linear activations => derivative 1) */
        for(int L = (int)n_layers_total-2; L>=1; L--){
            size_t cur_n = sizes[L];
            size_t next_n = sizes[L+1];
            size_t cur_off = offset[L] - sizes[0];
            size_t next_off = offset[L+1] - sizes[0];
            /* initialize */
            for(size_t i=0;i<cur_n;i++) deltas[cur_off + i] = 0.0;
            if(L == (int)n_hidden){
                /* next is output layer */
                h_layer_t *ol = nn->output_layer;
                for(size_t j=0;j<next_n;j++){
                    neuron_t *nxt = ol->neurons[j];
                    double dnext = deltas[next_off + j];
                    for(size_t i=0;i<cur_n;i++) deltas[cur_off + i] += dnext * nxt->w[i];
                }
            } else {
                h_layer_t *nxtl = nn->layers[L];
                for(size_t j=0;j<next_n;j++){
                    neuron_t *nxt = nxtl->neurons[j];
                    double dnext = deltas[next_off + j];
                    for(size_t i=0;i<cur_n;i++) deltas[cur_off + i] += dnext * nxt->w[i];
                }
            }
        }

        /* Update weights: for each layer 1..last, update neurons using their input activation and delta */
        for(size_t L=1; L<n_layers_total; L++){
            double *prev_ptr = &acts[offset[L-1]];
            size_t cur_n = sizes[L];
            size_t cur_off = offset[L] - sizes[0];
            if(L <= n_hidden){
                h_layer_t *hl = nn->layers[L-1];
                for(size_t j=0;j<cur_n;j++){
                    neuron_t *nj = hl->neurons[j];
                    double grad = deltas[cur_off + j];
                    neuron_update(nj, prev_ptr, grad, nn->params.learning_rate);
                }
            } else {
                h_layer_t *ol = nn->output_layer;
                for(size_t j=0;j<cur_n;j++){
                    neuron_t *nj = ol->neurons[j];
                    double grad = deltas[cur_off + j];
                    neuron_update(nj, prev_ptr, grad, nn->params.learning_rate);
                }
            }
        }

        /* Debug: compute and print L2 norm of all weights to detect collapse to zero */
        {
            double sum_sq_w = 0.0;
            /* hidden layers */
            for(size_t L=0; L<nn->n_layers; L++){
                h_layer_t *hl = nn->layers[L];
                for(size_t j=0;j<hl->n_neurons;j++){
                    neuron_t *n = hl->neurons[j];
                    for(size_t k=0;k<n->in_len;k++) sum_sq_w += n->w[k]*n->w[k];
                    sum_sq_w += n->b*n->b;
                }
            }
            /* output layer */
            h_layer_t *ol = nn->output_layer;
            for(size_t j=0;j<ol->n_neurons;j++){
                neuron_t *n = ol->neurons[j];
                for(size_t k=0;k<n->in_len;k++) sum_sq_w += n->w[k]*n->w[k];
                sum_sq_w += n->b*n->b;
            }
            double l2 = sqrt(sum_sq_w);
            LOG_INFO("[nn-debug] weights L2 norm = %f\n", l2);
        }

        double cost = sqrt(sum_sq);
    LOG_INFO("[nn] training: euclidean cost=%f\n", cost);

        /* recompute forward pass with updated weights to return the post-update prediction */
        for(size_t L=1; L<n_layers_total; L++){
            double *prev_ptr = &acts[offset[L-1]];
            size_t cur_n = sizes[L];
            if(L <= n_hidden){
                h_layer_t *hl = nn->layers[L-1];
                for(size_t j=0;j<cur_n;j++) acts[offset[L]+j] = neuron_forward(hl->neurons[j], prev_ptr);
            } else {
                h_layer_t *ol = nn->output_layer;
                for(size_t j=0;j<cur_n;j++) acts[offset[L]+j] = neuron_forward(ol->neurons[j], prev_ptr);
            }
        }
        /* denormalize updated outputs into out_raw */
        denormalize_output(&nn->params, out_norm, out_raw);

    nn_save_weights(nn, "data/nn_weights.bin");

        free(deltas);
        free(acts);
        free(sizes);
        free(offset);
        return cost;
    } else {
        /* no training: denormalize and return NAN (no cost computed) */
        denormalize_output(&nn->params, out_norm, out_raw);
        free(acts);
        free(sizes);
        free(offset);
        return NAN;
    }
}

int nn_save_weights(nn_t* nn, const char* filename){
    /* Ensure parent directory exists (minimal, only single-level dirs like "data/") */
    const char *slash = strrchr(filename, '/');
#ifdef _WIN32
    if(!slash) slash = strrchr(filename, '\\');
#endif
    if(slash){
        size_t dlen = (size_t)(slash - filename);
        if(dlen > 0){ char dir[512]; if(dlen < sizeof(dir)){ memcpy(dir, filename, dlen); dir[dlen]=0;
#ifdef _WIN32
                    _mkdir(dir);
#else
                    mkdir(dir, 0755);
#endif
                }
        }
    }
    FILE* f = fopen(filename,"wb"); if(!f) return -1;
    // write header: n_layers, neurons_per_layer
    if(fwrite(&nn->n_layers,sizeof(size_t),1,f)!=1) { fclose(f); return -1; }
    for(size_t i=0;i<nn->n_layers;i++) if(fwrite(&nn->neurons_per_layer[i],sizeof(size_t),1,f)!=1){ fclose(f); return -1; }
    // write hidden layers
    for(size_t i=0;i<nn->n_layers;i++) if(h_layer_write(f, nn->layers[i])!=0){ fclose(f); return -1; }
    // write output layer
    if(h_layer_write(f, nn->output_layer)!=0){ fclose(f); return -1; }
    fclose(f);
    return 0;
}

/* Skip a layer stored in file: advance file pointer over one h_layer entry (neurons + each neuron's data) */
static int skip_layer(FILE* fp){
    size_t n_neurons = 0, input_len = 0;
    if(fread(&n_neurons, sizeof(size_t), 1, fp) != 1) return -1;
    if(fread(&input_len, sizeof(size_t), 1, fp) != 1) return -1;
    for(size_t ni=0; ni<n_neurons; ni++){
        size_t in_len = 0;
        if(fread(&in_len, sizeof(size_t), 1, fp) != 1) return -1;
        /* skip weights (double * in_len) and bias (double) */
        long toskip = (long)(sizeof(double) * in_len + sizeof(double));
        if(fseek(fp, toskip, SEEK_CUR) != 0) return -1;
    }
    return 0;
}

int nn_load_weights(nn_t* nn, const char* filename){
    FILE* f = fopen(filename,"rb"); if(!f) return -1;
    size_t file_n_layers = 0;
    if(fread(&file_n_layers, sizeof(size_t), 1, f) != 1){ fclose(f); return -1; }

    /* read neurons_per_layer array from file */
    size_t *file_neurons = NULL;
    if(file_n_layers > 0){
        file_neurons = (size_t*)malloc(sizeof(size_t) * file_n_layers);
        if(!file_neurons){ fclose(f); return -1; }
        if(fread(file_neurons, sizeof(size_t), file_n_layers, f) != file_n_layers){ free(file_neurons); fclose(f); return -1; }
    }

    /* Log diagnostics */
    LOG_INFO("[nn] weights file: hidden_layers_in_file=%zu, expected=%zu\n", file_n_layers, nn->n_layers);
    for(size_t i=0;i<file_n_layers;i++) LOG_INFO("[nn] file layer %zu neurons=%zu\n", i, file_neurons[i]);

    /* Determine how many hidden layers we can load (prefix match) */
    size_t prefix = (file_n_layers < nn->n_layers) ? file_n_layers : nn->n_layers;
    for(size_t i=0;i<prefix;i++){
        if(file_neurons[i] != nn->neurons_per_layer[i]){
            LOG_ERROR("[nn] layer size mismatch at index %zu: file=%zu expected=%zu\n", i, file_neurons[i], nn->neurons_per_layer[i]);
            free(file_neurons); fclose(f); return -1;
        }
    }

    /* helper to skip an unrelated layer in the file (reads structure and advances file ptr) */
    /* implemented as file-scope static skip_layer() above */

    /* Now read hidden layers: for file layers, if they map to our nn->layers read into them; else skip */
    for(size_t i=0;i<file_n_layers;i++){
        if(i < nn->n_layers){
            if(h_layer_read(f, nn->layers[i]) != 0){ free(file_neurons); fclose(f); return -1; }
        } else {
            if(skip_layer(f) != 0){ free(file_neurons); fclose(f); return -1; }
        }
    }

    /* attempt to read output layer from file; only accept if its sizes match */
    int output_loaded = 0;
    if(h_layer_read(f, nn->output_layer) == 0){
        output_loaded = 1;
    } else {
        LOG_INFO("[nn] output layer in file did not match expected output layer; leaving random output layer\n");
        /* not fatal; we may still have loaded hidden layers */
    }

    free(file_neurons);
    fclose(f);

    if(prefix > 0 || output_loaded) {
        LOG_INFO("[nn] loaded %zu hidden layers from file (output_loaded=%d)\n", prefix, output_loaded);
        return 0;
    }
    return -1;
}
