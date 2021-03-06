/* 
C function to perform frequency renormalization on brain imaging data

Call in main as:

FT_intensity(intensity,NC,NT);

where

 -- intensity is a matrix of intensity data (input); the M x N matrix is vectorized, so that the matrix entry  (i,j)  is stored in array element [i+M*j] 

 -- NC is the number of voxels
 -- NT is the number of time points



FOR EACH VOXEL, the following tranformations are applied:
1) the intensity time series undergoes discrete Fourier transform (DFT) 
2) each Fourier component is renormalized by the square root of the corresponding power, so as to enhance high frequency components
3) all Fourier components are renormalized by the maximum power (maximum over all Fourier components) so as to renormalize the signal of each voxel
4) the resulting renormalized Fourier components are back-trasformed to real time space

*/

#include "stdio.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;


void FT_intensity(double *intensity, int NC, int NT) {


 void DFT(int NT, double*, double*,double*,double*,double*,double*,bool);   // declares subroutine that performs DFT


 int omega=int(NT/2)+1;  // number of intependent components in FT

 double Pi=M_PI;   
 double p=2*Pi/NT; // minimum frequency for FT

 double *cos_ux=new double[NT]; // cosines for FT (given as input to the  'DFT' routine so that they don't get computed at each call)
 double *sin_ux=new double[NT]; // sines for FT (given as input to the  'DFT' routine so that they don't get computed at each call)

 for(int i2=0; i2<NT; i2++) {
     cos_ux[i2]=cos(p*i2);
     sin_ux[i2]=sin(p*i2);
 }

 double *ftr=new double[NT];  // intensity for each scan (real part)
 double *fti=new double[NT];  // intensity  of each scan (imaginary part) 
 double *frr=new double[NT];   // intensity for each frequency  after FT (real part) 
 double *fri=new double[NT];   // intensity for each frequency  after FT (imaginary part) 

 double *fr=new double[omega*NC]; //  intensity for each frequency  after FT (real part)
 double *fi=new double[omega*NC]; //  intensity for each frequency  after FT (real part)
 double *power=new double[omega]; //  power at each frequency, averaged over voxels
 double *pow_of_om=new double[omega];  // power at each frequency for each voxel

 // compute DFT for each voxel. Re(FT)=fr Im(FT)=fi

 for(int i1=0; i1<NC; i1++) {
     for(int i2=0; i2<NT; i2++) {
         frr[i2]=intensity[NT*i1+i2]; 
         fri[i2]=0;   
     }
     
     DFT(NT,frr,fri,ftr,fti,cos_ux,sin_ux,true);
     
     for(int j=0; j<omega; j++) {
         fr[omega*i1+j]=ftr[j];
         fi[omega*i1+j]=fti[j];
     }
 }

// compute average power over voxels 


 for(int j=0; j<omega; j++) {
     power[j]=0;
     for(int i1=0; i1<NC; i1++) power[j]=power[j]+fr[omega*i1+j]*fr[omega*i1+j]+fi[omega*i1+j]*fi[omega*i1+j];
     power[j]=power[j]/NC;
 }


 // renormalize DFT components

 for(int i1=0; i1<NC; i1++) {


     fr[omega*i1+0]=0;
     fi[omega*i1+0]=0;
     pow_of_om[0]=0;

     double max_pow_of_om=-100;  // maximum power for each voxel 

     for(int j=0; j<omega; j++) {
   
          // divides each DFT component by the square root of the average power at the same frequency
         fr[omega*i1+j]=fr[omega*i1+j]/sqrt(power[j]);
         fi[omega*i1+j]=fi[omega*i1+j]/sqrt(power[j]);

          // computes maximum power for each voxel
         pow_of_om[j]=sqrt(fr[omega*i1+j]*fr[omega*i1+j]+fi[omega*i1+j]*fi[omega*i1+j]);
         if(pow_of_om[j]> max_pow_of_om)  max_pow_of_om =pow_of_om[j];
     }

     // divides the DFT components by the square root of the maximum power at each voxles
     for(int j=0; j<omega; j++) {
         fr[omega*i1+j]=fr[omega*i1+j]/max_pow_of_om;
         fi[omega*i1+j]=fi[omega*i1+j]/max_pow_of_om;
       } 

     // perform inverse DFT

     ftr[0]=0;
     fti[0]=0;

     for(int j=1; j<omega; j++) {
         ftr[j]=fr[omega*i1+j];
         ftr[NT-j]=ftr[j];
         fti[j]=fi[omega*i1+j];
         fti[NT-j]=-fti[j];
     }

     DFT(NT,frr,fri,ftr,fti,cos_ux,sin_ux,false);

    // update intensities given by inverse FT 

     for(int i2=0; i2<NT; i2++) {
         intensity[NT*i1+i2]=frr[i2]; 
     }
 }
  
}
