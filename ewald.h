#ifndef EWALD_H
#define EWALD_H

#include "p3m.h"

void Ewald_init(int);
void Ewald_k_space(FLOAT_TYPE, int);
void Ewald_compute_influence_function(FLOAT_TYPE);
FLOAT_TYPE Ewald_compute_optimal_alpha(FLOAT_TYPE, int);
FLOAT_TYPE Ewald_estimate_error(FLOAT_TYPE, FLOAT_TYPE, int);

double Ewald_error_wrapper(double a, int *b, int c, int NP, double e, double alpha_L, double r_cut_iL, double *box_l);

#endif
