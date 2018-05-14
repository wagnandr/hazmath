/*! \file  marking.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 20170715.
 *  Copyright 2017__HAZMATH__. All rights reserved.
 *
 *   \note routines to mark simplices for refinement
*/

#include "hazmath.h"
void marks(INT level,scomplex *sc)
{
  /* mark simplices depending on te value of an estimator */
  /* the estimator here is the aspect ratio of the simplex */
  INT n=sc->n,n1=n+1,ns=sc->ns,nv=sc->nv;
  INT kbadel,ke,i,j,j1,k,p,q,ni,node1,node2,mj,mk;
  INT ne=(INT )((n*n1)/2);
  REAL slmin,slmax,asp,aspmax=-10.;;
  REAL *sl=(REAL *)calloc(ne,sizeof(REAL));
  INT kbad=0;
  for(i = 0;i<ns;i++){
    if(sc->gen[i] <level) continue;
    ni=n1*i;
    ke=0;
    for (j = 0; j<n;j++){      
      mj=sc->nodes[ni+j]*n;
      j1=j+1;
      for (k = j1; k<n1;k++){
	mk=sc->nodes[ni+k]*n;
	sl[ke]=0e0;
	for(p=0;p<n;p++){
	  sl[ke]+=(sc->x[mj+p]-sc->x[mk+p])*(sc->x[mj+p]-sc->x[mk+p]);
	}
	sl[ke]=sqrt(sl[ke]);
	ke++;
      }
    }
    slmin=sl[0];
    slmax=sl[0];
    for(j=1;j<ne;j++){
      if(sl[j]<slmin) slmin=sl[j];
      if(sl[j]>slmax) slmax=sl[j];
    }
    asp=slmax/slmin;
    if(asp>1e1){
      sc->marked[i]=1;
      kbad++;
      //      fprintf(stdout,"\nlev=%d, gen=%d, asp= %e(%e/%e)\n",level,sc->gen[i],asp,slmax,slmin);
      if(asp>aspmax){kbadel=i;aspmax=asp;}
    }
  }
  //  fprintf(stdout,"\nbad:%d, aspectmax=%e (at el=%d)\n",kbad,aspmax,kbadel);
  //  exit(33);
  if(sl)free(sl);
  return;
}
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
void markstar(INT level,scomplex *sc, INT nstar, REAL *xstar)
{
  /* 
     from marked simplices, remove any simplex that does not contain a
     point from xstar[...]. simplices which are initially unmarked are
     left unmarked
  */
  //  fprintf(stdout,"\nNSTAR=%d\n",nstar);fflush(stdout);
  INT n=sc->n,n1=n+1,ns=sc->ns;
  INT istar,jstar,j=-1;
  if(!nstar){
    /* mark everything and bail out. */
    for(j=0;j<ns;j++){
      sc->marked[j]=TRUE;
    }
    return;
  }
  INT *scnjn=NULL,mrkno=0,mrktst=0,flagmrk=0;  
  REAL *xstar0=NULL;
  for(j=0;j<ns;j++){
    flagmrk=0;
    if(sc->child0[j]>=0) {continue;}
    if(!sc->marked[j]) {continue;}
    mrktst++;
    scnjn = sc->nodes+j*n1;
    for(jstar=0;jstar<nstar;jstar++){
      //      fprintf(stdout,"\nel=%d\n",jstar+1);
      xstar0=xstar+jstar*n;
      if(!xins(n,scnjn,sc->x,xstar0)){
	//	fprintf(stdout,"\nel=%d, found: %d; NOmrk=%d",j,jstar+1,mrkno);
	mrkno--;
	sc->marked[j]=jstar+1;
	flagmrk=1;
	break;
      }
    }
    if(flagmrk) continue;
    mrkno++;
    sc->marked[j]=FALSE;
  }
  /* fprintf(stdout,							\ */
  /* 	  "\nat level %d: UN-MARKING %d elements out of %d tested.",	\ */
  /* 	  level, mrkno,mrktst); */
  return;
}
