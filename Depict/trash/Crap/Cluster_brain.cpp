/*
C++ function to apply the clustering algorithm [A. Rodriguez, A. Laio, Science 344 (6191), 1492-1496] to brain imaging data, clustering together voxels with similar time-series

Call in MATLAB as:

 [final_assignation,density,dist_to_higher]=Cluster_brain(coords,scal,intensity,[average_density,coherent_neighbors_cut,density_cut,max_number_clusters,interactive,connected_cut]);

where:

 -- 'final_assignation' is a vector containing cluster assignations of each voxel (output)
 -- 'density' is a vector containing the densities of each voxel (output)
 -- 'dist_to_higher' is a vector containing the deltas (distance from voxel with higher density) of each voxel (output)
 
 -- 'coords' is a matrix of coordinate data (input): the M x N matrix is vectorized, so that the matrix entry  (i,j)  is stored in array element [i+M*j] 
 -- 'scal' is 3 x 1 vector expressing the voxel sizes
 -- 'intensity' is a matrix of intensity data (input); the M x N matrix is vectorized, so that the matrix entry  (i,j)  is stored in array element [i+M*j] 
 -- 'average_density' is the average number of neighbors (which fixes d_c)
 -- 'coherent_neighbors_cut' is the coherent neighbors threshold to remove noise
 -- 'density_cut' excludes as cluster centers points with density lower than this threshold
 -- 'max_number_clusters' fixes the maximum number of clusters
 -- 'interactive' is an option to display the decision graph and allow the user to choose the number of clusters the minimum density of cluster centers accordingly
 -- 'connected_cut' is an option to exclude small disconnected regions from clusters

*/


#include "stdio.h"
#include "mex.h"
#include <cmath>
#include <cstdlib>
#include <cstring>


using namespace std;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

 double *coords=mxGetPr(prhs[0]);   // input coordinate data matrix
 double *scal=mxGetPr(prhs[1]); // input voxel size matrix
 double *intensity=mxGetPr(prhs[2]); // input intensity data matrix
 double *constants=mxGetPr(prhs[3]); // input options 


 int Ncut=(int)constants[0]; // average number of neighbors
 int SPATIALCUT=(int)constants[1]; // coherent neighbors threshold
 int NNC_min=(int)constants[2]; // minimum density for cluster centers
 int NCLUST=(int)constants[3]; // number of clusters
 bool interactive=(bool)constants[4]; // choose NCLUST and NNC_min interactively
 bool connectedcut=(bool)constants[5]; // cut small disconected regions from clusters

 int NC=mxGetN(prhs[2]);  // number of voxels
 int NT=mxGetM(prhs[2]);  // number of time scans

 plhs[0] = mxCreateDoubleMatrix(1,NC,mxREAL);
 double *NNC=mxGetPr(plhs[0]); // number of neighbors for each voxel

 plhs[1] = mxCreateDoubleMatrix(1,NC,mxREAL);
 double *dist_to_higher=mxGetPr(plhs[1]); // for each voxel, minimum distance from a voxel with higher density

 plhs[2] = mxCreateDoubleMatrix(1,NC,mxREAL);
 double *final_assignation=mxGetPr(plhs[2]); // output cluster assignations

 srand(2);


//************************************** FISSA IL VALORE DI DC ********************************************// 

 mexPrintf("fixing dc based on average density \n");
 mexEvalString("drawnow;");

 double dist_c=0.1; // dc=distance to be considered neighbors
 int r; // random voxel
 int NNt; // number of neighbors at a distance < dc

 double dist;

 // chooses dc such that the average number of voxels is Ncut

 bool corr_avn=false; // true if the average number of voxels is approximately equal to Ncut

 double coeff1=1.2;   
 double coeff2=1.05;  

 int i1,i2,i3,k,l,ii,it;
 

 while(corr_avn==false) {
 
     NNt=0; // average number of  neighbors
  
     // it takes 500  random voxels, for each one it computes the number of neighbors (at a distance < dc), then it averages this number over the  500 voxels 
 
     for(k=0; k<500; k++) {
         r=rand();
         r=r%NC; // random voxel
         for(i1=0; i1<NC; i1++) {
             dist=0;
             for(i2=0; i2<NT; i2++) {
                 dist=dist+(intensity[NT*r+i2]-intensity[NT*i1+i2])*(intensity[NT*r+i2]-intensity[NT*i1+i2]);
             }
             dist=sqrt(dist/((double)NT));
             if(dist < dist_c) NNt++;
         }
     }

     NNt=(int)((double)NNt/500.0);
     mexPrintf("dist_c %f\t%d\n", dist_c,NNt);
     mexEvalString("drawnow;");
     if(NNt > Ncut*coeff1) dist_c=dist_c/coeff2;       // if the average number of nieghbors is too low it raises  dc
     else if(NNt < Ncut/coeff1) dist_c=dist_c*coeff2;  // if the average number of nieghbors is too higher it lowers  dc
     else corr_avn=true;
 }


//************************************** COHERENT NEIGHBORS COMPUTATION ********************************************// 

 mexPrintf("compute coherent neighbors \n");
 mexEvalString("drawnow;");

 const int CHUNK=1000;

 int *NNR=new int[NC]; // number of neighbors for each voxel, including only voxels  which are spatially close (r < sqrt(3)+epsilon)
 int *NN_LIST=new int[27*NC];   // list of spatially close voxels belonging to the same cluster, for each voxel
 double ri1i3;  // spatial distance between voxels
 bool kill[NC]; // selects voxels to be killed by coherent neighbors filter

 for(i1=0; i1<NC; i1++) {
     NNR[i1]=0;
 }


 scal[0]=scal[0]*2;
 scal[1]=scal[1]*2;


 for(i1=0; i1<NC; i1++) {
     for(i3=i1+1; i3<NC; i3++) {
         dist=0;	
         for(i2=0; i2<NT; i2++) {
             dist=dist+(intensity[NT*i3+i2]-intensity[NT*i1+i2])*(intensity[NT*i3+i2]-intensity[NT*i1+i2]);
         }
         dist=sqrt(dist/((double)NT));
         if(dist<dist_c) {
             ri1i3=0;
             for(l=0; l<3; l++) {
                 ri1i3=ri1i3+(coords[3*i1+l]-coords[3*i3+l])*(coords[3*i1+l]-coords[3*i3+l])/(scal[l]*scal[l]);
             }
             ri1i3=sqrt(ri1i3);
             if(ri1i3<sqrt(3.0)+0.01) {  // it only counts voxels which are spatially close  (r < sqrt(3)+epsilon)
                NNR[i1]++;
                NNR[i3]++;
             }
         } 
     }
 }


 // it remodulates the number of neighbors  amplifying the signal of spatially close voxels
 
 for(i1=0; i1<NC; i1++) {
     if(NNR[i1] < SPATIALCUT) kill[i1]=true;
 }

 
//************************************** DENSITY COMPUTATION ********************************************// 

 mexPrintf("compute local density \n");
 mexEvalString("drawnow;");


 for(i1=0; i1<NC; i1++) {
     NNC[i1]=0;
 }



 for(i1=0; i1<NC; i1++) {
     if(kill[i1]==true) continue;
     for(i3=i1+1; i3<NC; i3++) {
        if(kill[i3]==true) continue;
         dist=0;	
         for(i2=0; i2<NT; i2++) {
             dist=dist+(intensity[NT*i3+i2]-intensity[NT*i1+i2])*(intensity[NT*i3+i2]-intensity[NT*i1+i2]);
         }
         dist=sqrt(dist/((double)NT));
         if(dist<dist_c) {
             NNC[i1]++;
             NNC[i3]++;
         } 
     }
 }

 
 mexPrintf("done\n");
 mexEvalString("drawnow;");


//************************************** FIND THE DECISION GRAPH ********************************************// 

 mexPrintf("computing mindist on voxels  \n");
 mexEvalString("drawnow;");

 int *i3_closest=new int[NC];     //for each voxel, the closest voxel with a higher density

 for(i1=0; i1<NC; i1++) {
     dist_to_higher[i1]=0;
 }


 // to avoid degeneracy  (if two points have same density, then it can be a problem below when looking for nearest points of higher density)  

 double rr; // for randomization on NNC

 for(i1=0; i1<NC; i1++) {
     if(NNC[i1]==0.) continue;
     rr=((double)rand())/RAND_MAX;
     NNC[i1]=NNC[i1]+rr; 
 }

 double dist_m; // minimum distance from voxel with higher density

 for(i1=0; i1<NC; i1++) {
     /*if(i1%(NC/20)==0) { mexPrintf(".");  mexEvalString("drawnow;"); }*/
     if(NNC[i1]==0.) {
         i3_closest[i1]=-1;
         continue;
     }
     dist_m=10.;
     for(i3=0; i3<NC; i3++) {
         if(i3==i1) continue;
         else if(NNC[i3]<NNC[i1]) continue;
         dist=0;
         for(i2=0; i2<NT; i2++) {
             dist=dist+(intensity[NT*i3+i2]-intensity[NT*i1+i2])*(intensity[NT*i3+i2]-intensity[NT*i1+i2]);
         }
         dist=sqrt(dist/((double)NT));
         if(dist < dist_m) {
            dist_m=dist;
            i3_closest[i1]=i3;
         }
     }
     dist_to_higher[i1]=dist_m;
 }


 double dist_to_higher_max=0;    // minimum distance from voxel with higher density, maximized over all voxels 

 for(i1=0; i1<NC; i1++) {
     if(dist_to_higher[i1] > dist_to_higher_max && dist_to_higher[i1]<10.) dist_to_higher_max = dist_to_higher[i1];
 }

 // it arbitrarily defines "minimum distance" for the absolute maximum of density

 for(i1=0; i1<NC; i1++) {
     if(dist_to_higher[i1]==10.) {
         dist_to_higher[i1]=1.1*dist_to_higher_max;
         i3_closest[i1]=i1;
     }
 }


 mexPrintf("done\n");
 mexEvalString("drawnow;");



 mxArray *plotdata[3];
 plotdata[0] = plhs[0];
 plotdata[1] = plhs[1];
 plotdata[2]=mxCreateString("o");

 if(interactive==true) {
     mexCallMATLAB(0,NULL,3,plotdata,"plot");
     mexEvalString("drawnow;");
     mxArray *input_from_keyboard[2];
     mxArray *output_to_keyboard[2];
     input_from_keyboard[0]=mxCreateString("input number of clusters and press ENTER\n");
     mexCallMATLAB(1,output_to_keyboard,1,input_from_keyboard,"input");
     NCLUST=(int)mxGetScalar(output_to_keyboard[0]);
     mxDestroyArray(input_from_keyboard[0]);
     mxDestroyArray(output_to_keyboard[0]);
     input_from_keyboard[0]=mxCreateString("input minimum density of cluster centers and press ENTER\n");
     mexCallMATLAB(1,output_to_keyboard,1,input_from_keyboard,"input");
     NNC_min=(int)mxGetScalar(output_to_keyboard[0]);
     mxDestroyArray(input_from_keyboard[0]);
     mxDestroyArray(output_to_keyboard[0]);
 }


/************************************** FIND CLUSTERS**************************************/


 int *clust=new int[NC]; // cluster to which each voxel belongs
 int *clustn=new int[NC];  // cluster to which each voxel belongs, after cluster reordering 
 int *nocc=new int[NCLUST];  // number of voxels assigned to each cluster
 double *NNCclust=new double[NCLUST];  // average density each cluster
 int *iicenter=new int[NCLUST]; // voxels  corresponding to cluster centers
 int *NNC_center=new int[NCLUST]; // density in the cluster centers
 double dhcenter[NC]; // dist_to_higher for cluster centers  


 for(i1=0; i1<NC; i1++) {
     //exclude as possible centers voxels that have a density smaller than NNC_min
     if(NNC[i1]<NNC_min) dist_to_higher[i1]=0;
 }

 int count=0;

 // finds cluster centers

 while(count < NCLUST) {
     
     dist_to_higher_max=0;
     int max_loc;

     for(i1=0; i1<NC; i1++) {
         if(dist_to_higher[i1]> dist_to_higher_max) {
             dist_to_higher_max=dist_to_higher[i1];
             max_loc=i1;
         }
     }
     iicenter[count]=max_loc;
     dhcenter[count]=dist_to_higher[iicenter[count]];
     dist_to_higher[iicenter[count]]=0;
     count++;
 }

 for(ii=0; ii<NCLUST; ii++) {
     dist_to_higher[iicenter[ii]]=dhcenter[ii];
 }


// assign the centers to different clusters 

 for(i1=0; i1<NC; i1++) {
     clust[i1]=-1;
 }

 for(ii=0; ii<NCLUST; ii++) {
     clust[iicenter[ii]]=ii;
 }

 mexPrintf("primary assignation \n");
 mexEvalString("drawnow;");


//   assign each voxel to the same cluster of its nearest neighbour of higher density

 for(i1=0; i1<NC; i1++) {
     if(clust[i1]>-1) continue;
     if(NNC[i1]==0) continue;
     int i3=i1;

     while(clust[i1]==-1) {
         i3=i3_closest[i3];
         if(clust[i3]>-1) clust[i1]=clust[i3];
     }
 }


 mexPrintf("done\n");
 mexEvalString("drawnow;");


/******************************************* CONNECTED REGION DIMENSION ***********************************************************************/

 int *inn=new int[NC]; // number of spatial first neighbours belonging to the same cluster     
 bool *visited=new bool[NC];

 if(connectedcut==true) {

 // it determines fro each voxel the spatially close voxels belonging to the same cluster

     mexPrintf("compute neighbors in same cluster for each voxel\n");
     mexEvalString("drawnow;"); 
 
     for(i1=0; i1<NC; i1++) {
         NNR[i1]=0;
         for(i2=0; i2<27;i2++) {
             NN_LIST[27*i1+i2]=-1;
         }
         if(clust[i1]==-1) continue;
         for(i3=0; i3<NC; i3++) {
             if(clust[i1]==clust[i3]) { 
                 ri1i3=0;
                 for(l=0; l<3; l++) {
                     ri1i3=ri1i3+(coords[3*i1+l]-coords[3*i3+l])*(coords[3*i1+l]-coords[3*i3+l])/(scal[l]*scal[l]);
                 }
                 ri1i3=sqrt(ri1i3);
                 if(ri1i3 <sqrt(2)+0.01) {              // sqrt(2) --> up to 2nd neighbors 
                     NN_LIST[27*i1+NNR[i1]]=i3;
                     NNR[i1]=NNR[i1]+1;
                 }
             }
         } 
     }

     mexPrintf("done\n");
     mexEvalString("drawnow;"); 

 //  compute the dimension of a connected region of voxels assigned to the same cluster

     mexPrintf("compute connected regions \n");
     mexEvalString("drawnow;"); 


     int tot_vis;  // dimension of connected region of voxels assigned to the same cluster

     int Niter=200000;

     for(i1=0; i1<NC; i1++) {
         inn[i1]=0;
     }

     for(i1=0; i1<NC; i1++) {
         if(clust[i1]==-1) continue;
         else if(inn[i1]> 0) continue;
         for(i3=0; i3<NC; i3++) {
             visited[i3]=0;
         }
         int i3=i1;
         for(it=0; it<Niter; it++) {
             visited[i3]=1;
             int r=rand();
             r=r%(NNR[i3]);
             i3=NN_LIST[27*i3+r];
         }
         tot_vis=0;
         for(i3=0; i3<NC; i3++) {
             tot_vis=tot_vis+(int)visited[i3];
         }
         for(i3=0; i3<NC; i3++) {     
             inn[i3]=inn[i3]+tot_vis*visited[i3];
         }
     }

     mexPrintf("done\n");
     mexEvalString("drawnow;"); 

     //  exclude the voxels belonging to connected regions smaller than thresold

     int NCUT_conn=5; 


     for(i1=0; i1<NC; i1++) { 
         if(inn[i1]<NCUT_conn) clust[i1]=-1;
     }

 }

 mexPrintf("reorder clusters \n");
 mexEvalString("drawnow;"); 

  // count average density of each cluster


 for(ii=0; ii<NCLUST; ii++) { 
     NNCclust[ii]=0;
 }

 for(i1=0; i1<NC; i1++) { 
     if(clust[i1]>-1) NNCclust[clust[i1]]=NNCclust[clust[i1]]+NNC[i1];
 }

 for(ii=0; ii<NCLUST; ii++) { 
     NNCclust[ii]=0;
     nocc[ii]=0;
 }

 for(i1=0; i1<NC; i1++) { 
     if(clust[i1]>-1) { 
         nocc[clust[i1]]++;
         NNCclust[clust[i1]]=NNCclust[clust[i1]]+NNC[i1];
     }
 }

 for(ii=0; ii<NCLUST; ii++) { 
     NNCclust[ii]=NNCclust[ii]/nocc[ii];
 }

 //  reorder clusters according to their size
 
 for(i1=0; i1<NC; i1++) { 
     clustn[i1]=0;
 }

 int ic;  // cluster number corresponding to the maximum number of voxels
 int icn=0; //cluster index after reordering
 int NNCclust_max=0; // maximum number of voxels assigned to a cluster



 int N_CLUST_true=0;

 for(ii=0; ii<NCLUST; ii++) {
     if(NNCclust[ii]>0) N_CLUST_true++; 
 }

 for (int ii=0; ii<N_CLUST_true; ii++) {

     int max_loc;
     NNCclust_max=0;

     for(ii=0; ii<NCLUST; ii++) {
         if(NNCclust[ii]> NNCclust_max) {
             NNCclust_max=NNCclust[ii];
             max_loc=ii;
         }
     }
     ic=max_loc;
     mexPrintf("cluster of size %d found. clust=%d\n", NNCclust_max, ic+1);
     NNCclust[ic]=0; 
     clustn[ic]=icn;
     icn++;
 }

 // evaluate final assignation

 count=0;

 for(i1=0; i1<NC; i1++) {
     ic=-1;
     if(clust[i1]>-1) ic=clustn[clust[i1]];
     final_assignation[i1]=ic+1;
 }

//delete NNC, NNR, NN_LIST, dist_to_higher, i3_closest, clust, clustn, nocc, NNC_border, iicenter, NNC_center, inn, visited;


}