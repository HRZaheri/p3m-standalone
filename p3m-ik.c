
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fftw3.h>

#include "p3m.h"

/* Pi, weil man's so oft braucht: */
#define PI 3.14159265358979323846264

#define r_ind(A,B,C) ((A)*Mesh*Mesh + (B)*Mesh + (C))
#define c_ind(A,B,C) (2*Mesh*Mesh*(A)+2*Mesh*(B)+2*(C))

fftw_plan forward_plan;
fftw_plan backward_plan[3];

FLOAT_TYPE *Fmesh[3];
FLOAT_TYPE *F_K[3];

static void forward_fft(void);
static void backward_fft(void);

void forward_fft(void) {
  fftw_execute(forward_plan);
}

void backward_fft(void) {
  int i;
  for(i=0;i<3;i++)
    fftw_execute(backward_plan[i]);
}

static FLOAT_TYPE sinc(FLOAT_TYPE d)
{
#define epsi 0.1

#define c2 -0.1666666666667e-0
#define c4  0.8333333333333e-2
#define c6 -0.1984126984127e-3
#define c8  0.2755731922399e-5

  double PId = PI*d, PId2;

  if (fabs(d)>epsi)
    return sin(PId)/PId;
  else {
    PId2 = SQR(PId);
    return 1.0 + PId2*(c2+PId2*(c4+PId2*(c6+PId2*c8)));
  }
  return 1.0;
}


void Init_ik(int Teilchenzahl) {
  int l;
  Qmesh = (FLOAT_TYPE *) realloc(Qmesh, 2*Mesh*Mesh*Mesh*sizeof(FLOAT_TYPE));
  Gx = (int *)realloc(Gx, Teilchenzahl*sizeof(int));
  Gy = (int *)realloc(Gy, Teilchenzahl*sizeof(int));
  Gz = (int *)realloc(Gz, Teilchenzahl*sizeof(int));

  G_hat = (FLOAT_TYPE *) realloc(G_hat, Mesh*Mesh*Mesh*sizeof(G_hat));  

  F_K[0] = Fx_K;
  F_K[1] = Fy_K;
  F_K[2] = Fz_K;

  forward_plan = fftw_plan_dft_3d(Mesh, Mesh, Mesh, (fftw_complex *)Qmesh, (fftw_complex *)Qmesh, FFTW_FORWARD, FFTW_ESTIMATE);
  for(l=0;l<3;l++){
    Fmesh[l] = (FLOAT_TYPE *)realloc(Fmesh[l], 2*Mesh*Mesh*Mesh*sizeof(FLOAT_TYPE));  
    backward_plan[l] = fftw_plan_dft_3d(Mesh, Mesh, Mesh, (fftw_complex *)Fmesh[l], (fftw_complex *)Fmesh[l], FFTW_BACKWARD, FFTW_ESTIMATE);
  }
  
}

void Aliasing_sums_ik(int NX, int NY, int NZ, FLOAT_TYPE alpha,
				  FLOAT_TYPE *Zaehler, FLOAT_TYPE *Nenner)
{
  static int aliasmax = 1; /* Genauigkeit der Aliasing-Summe (2 ist wohl genug) */
  
  FLOAT_TYPE S1,S2,S3,U2;
  FLOAT_TYPE fak1,fak2,zwi;
  int    MX,MY,MZ;
  FLOAT_TYPE NMX,NMY,NMZ;
  FLOAT_TYPE NM2;

  fak1 = 1.0/(FLOAT_TYPE)Mesh;
  fak2 = SQR(PI/(alpha*Len));

  Zaehler[0] = Zaehler[1] = Zaehler[2] = *Nenner = 0.0;

  for (MX = -aliasmax; MX <= aliasmax; MX++) {
      NMX = nshift[NX] + Mesh*MX;
      S1   = sinc(fak1*NMX); 
      for (MY = -aliasmax; MY <= aliasmax; MY++) {
	NMY = nshift[NY] + Mesh*MY;
	S2   = S1*sinc(fak1*NMY);
	for (MZ = -aliasmax; MZ <= aliasmax; MZ++) {
	  NMZ = nshift[NZ] + Mesh*MZ;
	  S3   = S2*sinc(fak1*NMZ);
	  U2  = pow(S3, 2*(ip+1));
	  NM2 = SQR(NMX) + SQR(NMY) + SQR(NMZ);
          *Nenner += U2;

          zwi  = U2 * exp(-fak2*NM2)/NM2;
	  Zaehler[0] += NMX*zwi;
          Zaehler[1] += NMY*zwi;
          Zaehler[3] += NMZ*zwi;
	      
	}
      }
  }
}

 
void Influence_function_berechnen_ik(FLOAT_TYPE alpha)
{
  /*
    Berechnet die influence-function, d.h. sowas wie das Produkt aus
    fouriertransformierter Ladungsverschmierung und fouriertransformierter
    Greenschen Funktion.  (-> HOCKNEY/EASTWOOD optimal influence function!)
    alpha  : Ewald-Parameter.
    ip     : Ordnung des charge assigbment schemes.
  */

  int    NX,NY,NZ;
  FLOAT_TYPE Dnx,Dny,Dnz;
  FLOAT_TYPE fak1,dMesh,dMeshi;
  FLOAT_TYPE Zaehler[3]={0.0,0.0,0.0},Nenner=0.0;
  FLOAT_TYPE zwi;
  dMesh = (FLOAT_TYPE)Mesh;
  dMeshi= 1.0/dMesh;
  
  /* bei Zahlen >= Mesh/2 wird noch Mesh abgezogen! */
  for (NX=0; NX<Mesh; NX++)
    {
      for (NY=0; NY<Mesh; NY++)
	{
	  for (NZ=0; NZ<Mesh; NZ++)
	    {
              double ghc;
	      if ((NX==0) && (NY==0) && (NZ==0))
		G_hat[r_ind(NX,NY,NZ)]=0.0;
              else if ((NX%(Mesh/2) == 0) && (NY%(Mesh/2) == 0) && (NZ%(Mesh/2) == 0))
                G_hat[r_ind(NX,NY,NZ)]=0.0;
	      else
		{
		  Aliasing_sums_ik(NX,NY,NZ,alpha,Zaehler,&Nenner);
		  
		  Dnx = Dn[NX];  
		  Dny = Dn[NY];  
		  Dnz = Dn[NZ];
		  
		  zwi  = Dnx*Zaehler[0] + Dny*Zaehler[1] + Dnz*Zaehler[2];
		  zwi /= ( (SQR(Dnx) + SQR(Dny) + SQR(Dnz)) * SQR(Nenner) );

		  ghc = G_hat[r_ind(NX,NY,NZ)] = 2.0*Mesh*Mesh*Mesh * SQR(Leni) * zwi;
		}
              if(ghc > 1000.0)              
                fprintf(stderr, "n (%d %d %d), Dn (%lf %lf %lf) Z(%.15f %.15f %.15f) N(%.15f) => %lf\n", 
                        NX, NY, NZ, Dnx, Dny, Dnz, Zaehler[0], Zaehler[1], Zaehler[2], Nenner, ghc);
	    }
	}
    }
}


void P3M_ik(const FLOAT_TYPE alpha, const int Teilchenzahl)
{
  /*
    Berechnet den k-Raum Anteil der Ewald-Routine auf dem Gitter.
    Die Ableitung wird durch analytische Differentiation der charge assignment
    function erreicht, so wie es auch im EPBDLP-paper geschieht.
    alpha : Ewald-Parameter.
  */
  
  /* Zaehlvariablen: */
  int i, j, k, l, m, ii; 
  /* wahre n-Werte im Impulsraum: */
  int nx,ny,nz;
  /* Variablen fuer FFT: */
  int Nx,Ny,Nz,Lda, Ldb, sx, sy, sz, status;   
  /* Schnelles Modulo: */
  int MESHMASKE;
  /* Hilfsvariablen */
  FLOAT_TYPE dx,dy,dz,d2,H,Hi,dMesh,MI2,modadd2,modadd3;
  int modadd1;
  /* charge-assignment beschleunigen */
  FLOAT_TYPE T1,T2,T3,T4,T5;
  /* schnellerer Zugriff auf die Arrays Gx[i] etc.: */
  int Gxi,Gyi,Gzi;
  /* Argumente fuer das Array LadInt */
  int xarg,yarg,zarg;
  /* Gitterpositionen */
  int xpos,ypos,zpos;
  /* Soweit links vom Referenzpunkt gehts beim Ladungsver-
     teilen los (implementiert ist noch ein Summand Mesh!): */
  int mshift;
  FLOAT_TYPE dTeilchenzahli;
  
  //Pour le graphique des selfforces:
  int point,nb_points;
  FLOAT_TYPE posx=0,posy=0,posz=0;

  FILE *frho;

  Nx = Ny = Nz = Mesh;
  Lda = Ldb = Mesh; 
  sx = sy = sz = 1; 
  MESHMASKE = Mesh-1;
  dMesh = (FLOAT_TYPE)Mesh;
  H = Len/dMesh;
  Hi = 1.0/H;
  MI2 = 2.0*(FLOAT_TYPE)MaxInterpol;
  mshift = Mesh-ip/2;
  dTeilchenzahli = 1.0/(FLOAT_TYPE)Teilchenzahl;

    /* Vorbereitung der Fallunterscheidung nach geradem/ungeradem ip: */
  switch (ip)
    {
    case 0 : case 2 : case 4 : case 6 :
      { modadd1=ip/2;  
        modadd2=0.5;
        modadd3=0.0;
        break;
      }
    case 1 :case 3 : case 5 :
      { modadd1=(ip+1)/2 - 1; 
        modadd2=0.0;
        modadd3=0.5;
        break;
      }
    default : 
      {
	fprintf(stderr,"Wert von ip (ip=%d) ist nicht erlaubt!",ip);
	fprintf(stderr,"Programm abgebrochen!");
	exit(1);
      } break;
    }
    
  /* Initialisieren von Q_re und Q_im */
  for(i=0;i<(2*Mesh*Mesh*Mesh); i++)
    Qmesh[i] = 0.0;

  /* chargeassignment */
  for (i=0; i<Teilchenzahl; i++)
    {
      FLOAT_TYPE q_frac[2];
        q_frac[0] = q_frac[1] = 0.0;        

	//        printf("particle i at (%lf, %lf, %lf).\n", i, xS[i], yS[i], zS[i]);
        //printf("H: %lf, Hi: %lf, ip %d\n", H, Hi, ip);
        dx = Hi*xS[i];
        dy = Hi*yS[i];
        dz = Hi*zS[i];

	Gxi = (int)(dx+modadd2) - modadd1;
	Gyi = (int)(dy+modadd2) - modadd1;
	Gzi = (int)(dz+modadd2) - modadd1;

        //printf("Next meshpoint (%d %d %d).\n", (int)(dx+modadd2),   (int)(dy+modadd2), (int)(dz+modadd2));

        dx = H*(Gxi + modadd1) - (xS[i]);
        dy = H*(Gyi + modadd1) - (yS[i]);
        dz = H*(Gzi + modadd1) - (zS[i]);

        //printf("d[3] (%lf %lf %lf)\n", dx, dy, dz);

        xarg = (int)((dx - round(dx) + modadd3 )*MI2);
        yarg = (int)((dy - round(dy) + modadd3 )*MI2);
        zarg = (int)((dz - round(dz) + modadd3 )*MI2);
        

	//printf("Lower left point (%d %d %d)\n", Gxi, Gyi, Gzi);
        //printf("Distance (%lf, %lf, %lf)\n", xarg, yarg, zarg);

	  for (j=0; j<=ip; j++)
	    {
	      xpos = (Gxi+j)&MESHMASKE;
	      T1   = Q[i]*LadInt[j][xarg];
	      for (k=0; k<=ip; k++)
		{
		  ypos = (Gyi+k)&MESHMASKE;
		  T4   = LadInt[k][yarg];
		  T2   = T1   * T4;
		  for (l=0; l<=ip; l++)
		    {
		      zpos = (Gzi+l)&MESHMASKE;
		      T5   = LadInt[l][zarg];
		      T3   = T2   * T5;
		      Qmesh[c_ind(xpos,ypos,zpos)] += T3;
		    }
		}
	    }
    }

  /* Durchfuehren der Fourier-Hin-Transformationen: */

  forward_fft();

  for (i=0; i<Mesh; i++)
    for (j=0; j<Mesh; j++)
      for (k=0; k<Mesh; k++)
	{
          int c_index = c_ind(i,j,k);
	  T1 = G_hat[r_ind(i,j,k)];
	  Qmesh[c_index] *= T1;
	  Qmesh[c_index+1] *= T1;

          for(l=0;l<3;l++) {
            int k_dir;
            switch(l) {
 	      case 0:
                k_dir = i;
                break;
              case 1:
                k_dir = j;
                break;
	      case 2:
                 k_dir = k;
                break;
	    }
	    Fmesh[l][c_index]   = -Dn[k_dir]*Qmesh[c_index+1];
	    Fmesh[l][c_index+1] = Dn[k_dir]*Qmesh[c_index];
          }

	}
  
  /* Durchfuehren der Fourier-Rueck-Transformation: */

  backward_fft();
   
  /* Kraftkomponenten: */

  /* chargeassignment */
  for (i=0; i<Teilchenzahl; i++)
    {
      FLOAT_TYPE f_frac;

	int direction;
        dx = Hi*xS[i];
        dy = Hi*yS[i];
        dz = Hi*zS[i];

	Gxi = (int)(dx+modadd2) - modadd1;
	Gyi = (int)(dy+modadd2) - modadd1;
	Gzi = (int)(dz+modadd2) - modadd1;

        dx = H*(Gxi + modadd1) - (xS[i]);
        dy = H*(Gyi + modadd1) - (yS[i]);
        dz = H*(Gzi + modadd1) - (zS[i]);

        xarg = (int)((dx - round(dx) + modadd3 )*MI2);
        yarg = (int)((dy - round(dy) + modadd3 )*MI2);
        zarg = (int)((dz - round(dz) + modadd3 )*MI2);
        

	for(direction=0;direction<3;direction++) {       
            f_frac = 0.0;
	    for (j=0; j<=ip; j++)
	      {
	        xpos = (Gxi+j)&MESHMASKE;
                T1 = LadInt[j][xarg];
	        for (k=0; k<=ip; k++)
		  {
		    ypos = (Gyi+k)&MESHMASKE;
		    T4   = LadInt[k][yarg];
		    T2   = T1   * T4;
		    for (l=0; l<=ip; l++)
		    {
                      FLOAT_TYPE B;
		      zpos = (Gzi+l)&MESHMASKE;
		      T5   = LadInt[l][zarg];
		      T3   = T2   * T5;
                      B = Fmesh[direction][c_ind(xpos,ypos,zpos)] * T3/(Len*Len*Len);
 		      F_K[direction][i] -= B;
                      f_frac -= B;

		    }
		}
	    }
	  }
	
      }

    
//  for (i=0; i<Teilchenzahl; i++) {
//   printf("k-force[%i] = %1.9lf %1.9lf %1.9lf\n",i,Fx_K[i],Fy_K[i],Fz_K[i]);
//  }

  return;
}