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
