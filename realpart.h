#pragma once

#ifndef REALPART_H
#define REALPART_H

#include "p3m.h"

/* Calculate Real space part of the Ewald sum.
 * Used in all methods
 */

typedef struct {
  // number of particles in list
  int n;
  // postitions of neighbors
  vector_array_t *p;
  // charges of neighbors
  FLOAT_TYPE *q;
  // ids in system array of neighbors
  int *id;
} neighbor_list_t;

// functions for neighbor list algorithm

// Build particle neighbor list. WARNING: This is O(n^2).
void Init_neighborlist(system_t *, parameters_t *);
// Calculate realpart of forces using list.
void Realpart_neighborlist(system_t *, parameters_t *, forces_t *);

// functions for n2 algorithm

void Realteil(system_t *, parameters_t *, forces_t *);

#endif