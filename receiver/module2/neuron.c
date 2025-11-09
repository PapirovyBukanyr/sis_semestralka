/**
 * neuron.c
 *
 * Neuron implementation: activation, parameter storage, serialization and
 * helper functions used by the hidden-layer implementation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "neuron.h"

/**
 * Generate a random double in [-1.0, 1.0]
 * 
 * @return random double in [-1.0, 1.0]
 */
static double drand_unit(){ return (double)rand() / (double)RAND_MAX * 2.0 - 1.0; }

/**
 * Create and initialize a new neuron with random weights.
 * 
 * @param in_len number of inputs to the neuron
 * @return pointer to allocated neuron_t or NULL on error
 */
neuron_t* neuron_create(size_t in_len){
neuron_t* n = (neuron_t*)calloc(1,sizeof(neuron_t));
if(!n) return NULL;
n->in_len = in_len;
n->w = (double*)malloc(sizeof(double)*in_len);
if(!n->w){ free(n); return NULL; }
// init small random weights
for(size_t i=0;i<in_len;i++) n->w[i] = drand_unit()*0.1;
n->b = drand_unit()*0.1;
return n;
}

/**
 * Free a neuron and its resources.
 */
void neuron_free(neuron_t* n){ if(!n) return; free(n->w); free(n); }

/**
 * Compute neuron output: V = A*B + C (dot product + bias)
 * 
 * @param n pointer to neuron
 * @param input input array of length n->in_len
 * @return output value
 */
double neuron_forward(const neuron_t* n, const double* input){
	double s = 0.0;
	for(size_t i=0;i<n->in_len;i++) s += n->w[i] * input[i];
	s += n->b;
	return s; /* linear */
}

/**
 * Update neuron weights using gradient descent.
 * 
 * @param n pointer to neuron
 * @param input input array of length n->in_len
 * @param grad_out gradient w.r.t. output (dL/dV)
 * @param lr learning rate
 */
void neuron_update(neuron_t* n, const double* input, double grad_out, double lr){
	for(size_t i=0;i<n->in_len;i++){
		double g = grad_out * input[i];
		n->w[i] -= lr * g;
	}
	n->b -= lr * grad_out;
}

/**
 * Write a neuron to a file.
 * 
 * @param f file pointer opened for writing
 * @param n pointer to neuron
 * @return 0 on success, -1 on error
 */
int neuron_write(FILE* f, const neuron_t* n){
	if(fwrite(&n->in_len,sizeof(size_t),1,f)!=1) return -1;
	if(fwrite(n->w,sizeof(double),n->in_len,f)!=n->in_len) return -1;
	if(fwrite(&n->b,sizeof(double),1,f)!=1) return -1;
	return 0;
}

/**
 * Read a neuron from a file.
 * 
 * @param f file pointer opened for reading
 * @param n pointer to neuron (must be preallocated with correct in_len)
 * @return 0 on success, -1 on error
 */
int neuron_read(FILE* f, neuron_t* n){
	size_t in_len=0;
	if(fread(&in_len,sizeof(size_t),1,f)!=1) return -1;
	if(in_len != n->in_len) return -1; /* mismatch */
	if(fread(n->w,sizeof(double),n->in_len,f)!=n->in_len) return -1;
	if(fread(&n->b,sizeof(double),1,f)!=1) return -1;
	return 0;
}
