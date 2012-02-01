#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "types.h"

#include "p3m-common.h"

// Methods


#include "p3m-ik-i.h"
#include "p3m-ik.h"
#include "p3m-ad.h"
#include "p3m-ad-i.h"
#include "greens.h"

#include "ewald.h"

#include "interpol.h"

// Utils and IO

#include "io.h"

// Real space part

#include "realpart.h"

// Dipol correction

#include "dipol.h"

// Error calculation

#include "error.h"

// Helper functions for timings

#include "timings.h"

#include "generate_system.h"

// #define WRITE_FORCES

// #define FORCE_DEBUG
// #define CA_DEBUG




void usage ( char *name ) {
    fprintf ( stderr, "usage: %s <alpha_min> <alpha_max> <alpha_step> <method> <nparticles> <box_length> <cao> <mesh> <rcut> <aliasing_max>\n", name );
}




int main ( int argc, char **argv ) {
    int methodnr;

    FLOAT_TYPE alphamin,alphamax,alphastep, ref_prec;

    FILE* fout;

    system_t *system;
    method_t method;
    parameters_t parameters;
    data_t *data;
    forces_t *forces;

    error_t error;

    if ( argc != 11 ) {
        usage ( argv[0] );
        return 128;
    }

    // Inits the system and reads particle data and parameters from file.
    // system = Daten_einlesen ( &parameters, argv[1] );

    system = generate_system( FORM_FACTOR_RANDOM, atoi(argv[5]), atof(argv[6]), 1.0 );
    parameters.rcut = atof(argv[9]);
    parameters.cao = atoi(argv[7]);
    parameters.ip = parameters.cao - 1;
    parameters.cao3 =  parameters.cao* parameters.cao* parameters.cao;
    parameters.mesh = atoi(argv[8]);

    forces = Init_forces(system->nparticles);

    alphamin = atof ( argv[1] );
    alphamax = atof ( argv[2] );
    alphastep = atof ( argv[3] );

    methodnr = atoi ( argv[4] );

    P3M_BRILLOUIN = atoi(argv[10]);

    //Exakte_Werte_einlesen( system, argv[2] );
    ref_prec = Calculate_reference_forces( system, &parameters );


    if ( methodnr == method_ewald.method_id )
        method = method_ewald;
#ifdef P3M_IK_H
    else if ( methodnr == method_p3m_ik.method_id )
        method = method_p3m_ik;
#endif
#ifdef P3M_IK_I_H
    else if ( methodnr == method_p3m_ik_i.method_id )
        method = method_p3m_ik_i;
#endif
#ifdef P3M_AD_H
    else if ( methodnr == method_p3m_ad.method_id )
        method = method_p3m_ad;
#endif
#ifdef P3M_AD_I_H
    else if ( methodnr == method_p3m_ad_i.method_id )
        method = method_p3m_ad_i;
#endif
    else {
        fprintf ( stderr, "Method %d not know.", methodnr );
        exit ( 126 );
    }

    if ( ( method.Init == NULL ) || ( method.Influence_function == NULL ) || ( method.Kspace_force == NULL ) ) {
        fprintf ( stderr,"Internal error: Method '%s' (%d) is not properly defined. Aborting.\n", method.method_name, method.method_id );
        exit ( -1 );
    }

    fprintf ( stderr, "Using %s.\n", method.method_name );

    fout = fopen ( "out.dat","w" );

    printf ( "Init" );
    fflush(stdout);
    data = method.Init ( system, &parameters );
    printf ( ".\n" );

    printf ( "Init neighborlist" );
    Init_neighborlist ( system, &parameters, data );
    printf ( ".\n" );

    printf ( "# %8s\t%8s\t%8s\t%8s\t%8s\n", "alpha", "DeltaF", "Estimate", "R-Error", "K-Error" );
    for ( parameters.alpha=alphamin; parameters.alpha<=alphamax; parameters.alpha+=alphastep ) {
        method.Influence_function ( system, &parameters, data );  /* Hockney/Eastwood */

        Calculate_forces ( &method, system, &parameters, data, forces ); /* Hockney/Eastwood */

        error = Calculate_errors ( system, forces );

        if ( method.Error != NULL ) {
            double estimate =  method.Error ( system, &parameters );
            printf ( "%8lf\t%8e\t%8e\t %8e %8e\n", parameters.alpha, error.f / sqrt(system->nparticles) , estimate,
                     error.f_r / sqrt(system->nparticles), error.f_k / sqrt(system->nparticles));
	    if ( estimate < ref_prec )
	      fprintf ( stderr, "warning: estimated precision '%g' exeeds precision of the reference data '%g'.\n", estimate, ref_prec );
            fprintf ( fout,"% lf\t% e\t% e\t% e\t% e\n",parameters.alpha,error.f / sqrt(system->nparticles) , estimate, error.f_r / sqrt(system->nparticles), error.f_k / sqrt(system->nparticles) );
        } else {
            printf ( "%8lf\t%8e\t na\t%8e\t%8e\n", parameters.alpha,error.f / system->nparticles , error.f_r, error.f_k );
            fprintf ( fout,"% lf\t% e\t na\n",parameters.alpha,error.f / system->nparticles );
        }
#ifdef FORCE_DEBUG
        fprintf ( stderr, "%lf rms %e %e %e\n", parameters.alpha, error.f_v[0], error.f_v[1], error.f_v[2] );
#endif
        fflush ( stdout );
        fflush ( fout );
    }
    fclose ( fout );

    return 0;
}

