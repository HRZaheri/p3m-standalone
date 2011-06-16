#include "charge-assign.h"

#include <stdio.h>

#define r_ind(A,B,C) ((A)*Mesh*Mesh + (B)*Mesh + (C))
#define c_ind(A,B,C) (2*Mesh*Mesh*(A)+2*Mesh*(B)+2*(C))

void assign_charge(int id, FLOAT_TYPE q,
		       FLOAT_TYPE real_pos[3],
		       FLOAT_TYPE *p3m_rs_mesh)
{
  int d, i0, i1, i2;
  FLOAT_TYPE tmp0, tmp1;
  /* position of a particle in local mesh units */
  FLOAT_TYPE pos;
  /* 1d-index of nearest mesh point */
  int nmp;
  /* index for caf interpolation grid */
  int arg[3];
  /* index, index jumps for rs_mesh array */
  FLOAT_TYPE cur_ca_frac_val, *cur_ca_frac;
  int cf_cnt = id*cao3 - 1;

  int base[3];
  int i,j,k;
  FLOAT_TYPE MI2 = 2.0*(FLOAT_TYPE)MaxInterpol;   

  FLOAT_TYPE Hi = (double)Mesh/(double)Len;

  int MESHMASKE = Mesh - 1;
  double charge = 0.0;
    /* particle position in mesh coordinates */
    for(d=0;d<3;d++) {
      pos    = real_pos[d]*Hi;
      nmp = (int) pos;
      base[d]  = (nmp - ip/2 - (ip%2))%MESHMASKE;
      arg[d] = (int) ((pos - nmp)*MI2);
      ca_ind[3*id + d] = base[d];
    }

    for(i0=0; i0<cao; i0++) {
      i = (base[0] + i0)&MESHMASKE;
      tmp0 = LadInt[i0][arg[0]];
      for(i1=0; i1<cao; i1++) {
        j = (base[1] + i1)&MESHMASKE;
	tmp1 = tmp0 * LadInt[i1][arg[1]];
	for(i2=0; i2<cao; i2++) {
          k = (base[2] + i2)&MESHMASKE;
	  cur_ca_frac_val = q * tmp1 * LadInt[i2][arg[2]];
          cf[cf_cnt++] = cur_ca_frac_val;

	  p3m_rs_mesh[c_ind(i,j,k)] += cur_ca_frac_val;
          charge += cur_ca_frac_val;
	}
      }
    }
    //    printf("Assigned %lf total from particle %d\n", charge, id);
}

// assign the forces obtained from k-space 
void assign_forces(double force_prefac, FLOAT_TYPE *F, int Teilchenzahl, FLOAT_TYPE *p3m_rs_mesh) 
{
  int i,c,i0,i1,i2;
  int cp_cnt=0, cf_cnt=0;
  int *base;
  int j,k,l;
  int MESHMASKE = Mesh - 1;
  FLOAT_TYPE A,B;

  cf_cnt=0;
    for(i=0; i<Teilchenzahl; i++) { 
      base = ca_ind + 3*i;
      for(i0=0; i0<cao; i0++) {
        j = (base[0] + i0)&MESHMASKE;
        for(i1=0; i1<cao; i1++) {
          k = (base[1] + i1)&MESHMASKE;
          for(i2=0; i2<cao; i2++) {
            l = (base[2] + i2)&MESHMASKE;
            A = cf[cf_cnt];
            B = p3m_rs_mesh[c_ind(j,k,l)];
            F[i] -= force_prefac*A*B; 
	    cf_cnt++;
	  }
	}
      }
    }
}


