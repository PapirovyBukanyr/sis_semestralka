#include "nn.h"
#include "neuron.h"
#include "h_layer.h"
#include "nn_params.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
    srand((unsigned)time(NULL));
    // try to load saved weights from disk; ignore errors (fresh start if missing/mismatch)
    if(nn_load_weights(nn, "nn_weights.bin")==0){
        fprintf(stderr, "[nn] loaded weights from nn_weights.bin\n");
    } else {
        fprintf(stderr, "[nn] no weight file loaded (starting with random weights)\n");
    }
    return nn;
}

void nn_free(nn_t* nn){
    if(!nn) return;
    // try to save weights; best-effort
    if(nn_save_weights(nn, "nn_weights.bin")==0){
        fprintf(stderr, "[nn] saved weights to nn_weights.bin\n");
    } else {
        fprintf(stderr, "[nn] failed to save weights to nn_weights.bin\n");
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

void nn_predict_and_maybe_train(nn_t* nn, const data_point_t* in, const float* target_raw, float* out_raw){
    double input_norm[INPUT_SIZE];
    normalize_input(&nn->params, in, input_norm);
    double out_norm[OUTPUT_SIZE];
    nn_forward(nn, input_norm, out_norm);
    // if target provided, do simple online SGD using MSE
    if(target_raw){
        double target_norm[OUTPUT_SIZE];
        for(size_t i=0;i<OUTPUT_SIZE;i++) target_norm[i] = ((double)target_raw[i]) / nn->params.scales[i];
        // compute gradients and backprop only through linear layers (simple)
        // For simplicity we perform a single-layer delta update on output neurons (no deep backprop)
        for(size_t j=0;j<nn->output_layer->n_neurons;j++){
            double y = out_norm[j];
            double t = target_norm[j];
            double err = y - t; // dL/dy for 0.5*(y-t)^2
            // get inputs to output layer (we need forward through hidden to get last activations)
            // recompute last activation vector
            size_t prev_size = INPUT_SIZE;
            double *cur = (double*)malloc(sizeof(double)*1024);
            double *tmp_in = (double*)malloc(sizeof(double)*1024);
            for(size_t i=0;i<prev_size;i++) cur[i]=input_norm[i];
            for(size_t L=0; L<nn->n_layers; L++){
                size_t n_neu = nn->layers[L]->n_neurons;
                for(size_t jj=0;jj<n_neu;jj++) tmp_in[jj] = neuron_forward(nn->layers[L]->neurons[jj], cur);
                prev_size = n_neu;
                for(size_t jj=0;jj<prev_size;jj++) cur[jj]=tmp_in[jj];
            }
        // update output neuron j — add lightweight logging of a sample weight/bias
        {
        neuron_t* out_n = nn->output_layer->neurons[j];
        // print a compact debug message showing error and first weight before update
        fprintf(stderr, "[nn] training neuron %zu err=%f lr=%g w0(before)=%f b(before)=%f\n",
            j, err, nn->params.learning_rate, out_n->w[0], out_n->b);
        neuron_update(out_n, cur, err, nn->params.learning_rate);
        fprintf(stderr, "[nn] neuron %zu w0(after)=%f b(after)=%f\n",
            j, out_n->w[0], out_n->b);
        }
            free(cur); free(tmp_in);
        }
        // also update hidden layers weights by a very simple heuristic: nudge them by propagated error averaged
        // (proper backprop omitted for brevity)
        // save weights after update
        nn_save_weights(nn, "nn_weights.bin");
    }
    // denormalize
    denormalize_output(&nn->params, out_norm, out_raw);
}

int nn_save_weights(nn_t* nn, const char* filename){
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

int nn_load_weights(nn_t* nn, const char* filename){
    FILE* f = fopen(filename,"rb"); if(!f) return -1;
    size_t file_n_layers=0;
    if(fread(&file_n_layers,sizeof(size_t),1,f)!=1){ fclose(f); return -1; }
    if(file_n_layers != nn->n_layers){ fclose(f); return -1; }
    for(size_t i=0;i<nn->n_layers;i++){
        size_t nloc=0; if(fread(&nloc,sizeof(size_t),1,f)!=1){ fclose(f); return -1; }
        // put back to stream pointer
        fseek(f, -((long)sizeof(size_t)), SEEK_CUR);
        if(nloc != nn->neurons_per_layer[i]){ fclose(f); return -1; }
    }
    // rewind to start of layers after header
    fseek(f, sizeof(size_t)*(1+nn->n_layers), SEEK_SET);
    for(size_t i=0;i<nn->n_layers;i++) if(h_layer_read(f, nn->layers[i])!=0){ fclose(f); return -1; }
    if(h_layer_read(f, nn->output_layer)!=0){ fclose(f); return -1; }
    fclose(f);
    return 0;
}
