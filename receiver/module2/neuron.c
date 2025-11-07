#include "neuron.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>


static double drand_unit(){ return (double)rand() / (double)RAND_MAX * 2.0 - 1.0; }


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

void neuron_free(neuron_t* n){ if(!n) return; free(n->w); free(n); }

double neuron_forward(const neuron_t* n, const double* input){
	double s = 0.0;
	for(size_t i=0;i<n->in_len;i++) s += n->w[i] * input[i];
	s += n->b;
	return s; /* linear */
}

void neuron_update(neuron_t* n, const double* input, double grad_out, double lr){
	for(size_t i=0;i<n->in_len;i++){
		double g = grad_out * input[i];
		n->w[i] -= lr * g;
	}
	n->b -= lr * grad_out;
}

int neuron_write(FILE* f, const neuron_t* n){
	if(fwrite(&n->in_len,sizeof(size_t),1,f)!=1) return -1;
	if(fwrite(n->w,sizeof(double),n->in_len,f)!=n->in_len) return -1;
	if(fwrite(&n->b,sizeof(double),1,f)!=1) return -1;
	return 0;
}

int neuron_read(FILE* f, neuron_t* n){
	size_t in_len=0;
	if(fread(&in_len,sizeof(size_t),1,f)!=1) return -1;
	if(in_len != n->in_len) return -1; /* mismatch */
	if(fread(n->w,sizeof(double),n->in_len,f)!=n->in_len) return -1;
	if(fread(&n->b,sizeof(double),1,f)!=1) return -1;
	return 0;
}
