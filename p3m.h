#pragma once

#ifndef P3M_H
#define P3M_H

#include "common.h"

#define MaxInterpol (2*100096)
#define Maxip 6

// Struct holding method parameters.

typedef struct {
    FLOAT_TYPE alpha;
    FLOAT_TYPE rcut;
    FLOAT_TYPE prefactor;
    int        mesh;
    int        ip;
    int        cao;
    int        cao3;
    FLOAT_TYPE precision;
} parameters_t;

// Struct holding method data.

typedef struct {
    // Mesh size the struct is initialized for
    int mesh;
    // Influence function
    FLOAT_TYPE *G_hat;
    // Charge mesh
    FLOAT_TYPE *Qmesh;
    // Force mesh for k space differentiation
    vector_array_t *Fmesh;
    // Shifted kvectors (fftw convention)
    FLOAT_TYPE *nshift;
    // Fourier coefficients of the differential operator
    FLOAT_TYPE *Dn;
    // Derivatives of the charge assignment function for analytical differentiation
    FLOAT_TYPE *dQdx[2], *dQdy[2], *dQdz[2];
    // Cache for charge assignment
    int *ca_ind[2];
    FLOAT_TYPE *cf[2];
    // Array for interpolated charge assignment function
    FLOAT_TYPE **LadInt;
    FLOAT_TYPE **LadInt_;
} data_t;

// Flags for method_t

enum {
    METHOD_FLAG_none = 0,
    METHOD_FLAG_ik = 1, // Method uses ik-diff
    METHOD_FLAG_ad = 2, // Method uses ad
    METHOD_FLAG_interlaced = 4, // Method is interlaced
    METHOD_FLAG_nshift = 8, // Method needs precalculated shifted k-values
    METHOD_FLAG_G_hat = 16, // Method uses influence function
    METHOD_FLAG_Qmesh = 32, // Method needs charge mesh
    METHOD_FLAG_ca = 64 // Method uses charge assignment
};

// Common flags for all p3m methods for convinience
#define METHOD_FLAG_P3M (METHOD_FLAG_nshift | METHOD_FLAG_G_hat | METHOD_FLAG_Qmesh | METHOD_FLAG_ca)

// methode type

typedef struct {
    int  method_id;
    const char *method_name;
    char flags;
    data_t * ( *Init ) ( system_t *, parameters_t * );
    void ( *Influence_function ) ( system_t *, parameters_t *, data_t * );
    void ( *Kspace_force ) ( system_t *, parameters_t *, data_t *, forces_t * );
    double ( *Error ) ( system_t *, parameters_t * );
} method_t;

#endif
