/*! \file src/utilities/amr_utils.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 20190115.
 *  Copyright 2017__HAZMATH__. All rights reserved.
 *
 *  \note contains some utility functions for mesh refinement. 
 *
 */
#include "hazmath.h"
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx*/
INT aresame(INT *a, INT *b, INT n)
{
  /* 
     checks (n^2 algorithm) if two have the same elements (up to a
     permutation), if they are returns 1, otherwise 0
  */
  INT i,j,flag,ai,bj;
  for (i=0;i<n;i++){
    ai=a[i];
    flag=0;
    for(j=0;j<n;j++){
      bj=b[j];
      if(ai==bj){
	flag=1; break;
      }
    }
    if(!flag) return 0;
  }
  return 1;
}
/****************************************************************/
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx*/
INT aresamep(INT *a, INT *b, INT n, INT *p)
{
  /* checks (n^2 algorithm) if two have the same elements (up to a
     permutation); if they do, then returns 2 and p is the permutation
     a[i]=b[p[i]] if there is no permutation, i.e. p[i]=i, then returns 1*/
  INT i,j,ai,bj;
  INT flag=-1,iret=1;
  for (i=0;i<n;i++)p[i]=-1;    
  for (i=0;i<n;i++){
    ai=a[i];
    flag=0;
    for(j=0;j<n;j++){
      bj=b[j];
      if(ai==bj){
	p[i]=j;
	flag=1;
	if(p[i]!=i) iret=2;
	break;
      }
    }
    if(!flag) return 0;
  }
  return iret;
}
/****************************************************************/
INT xins(INT n, INT *nodes, REAL *xs, REAL *xstar)
{
  /* 
     In dimension "n" constructs the map from reference simplex to
     simplex with coordinates xs[0..n].  Then solves a linear system
     with partial pivoting to determine if a point given with
     coordinates xstar[0..n-1] is in the (closed) simplex defined by
     "nodes[0..n] and xs[0..n]"
  */  
  INT n1=n+1,i,j,l0n,ln,j1;
  INT *p=NULL;
  REAL *A=NULL,*xhat=NULL, *piv=NULL;
  A=(REAL *)calloc(n*n,sizeof(REAL));
  xhat=(REAL *)calloc(n,sizeof(REAL));
  piv=(REAL *)calloc(n,sizeof(REAL));
  p=(INT *)calloc(n,sizeof(INT));
  /* fprintf(stdout,"\nj=%d; vertex=%d\n",0+1,nodes[0]+1); fflush(stdout); */
  /* fprintf(stdout,"\nvertex=%d\n",nodes[0]+1); */
  /* for(i=0;i<n;i++){ */
  /*   fprintf(stdout,"xyz=%f ",xs[l0n+i]); */
  /* } */
  /* fprintf(stdout,"\n"); */
  l0n=nodes[0]*n;
  for (j = 1; j<n1;j++){
    /* grab the vertex */
    ln=nodes[j]*n;
    j1=j-1;
    for(i=0;i<n;i++){
      /*A_{ij} = x_{i,nodes(j)}-x0_{i,nodes(j)},j=1:n (skip 0)*/
      A[i*n+j1] = xs[ln+i]-xs[l0n+i];
    }
  }
  //  fflush(stdout);
  for(i=0;i<n;i++){
    xhat[i] = xstar[i]-xs[l0n+i];
  }
  //print_full_mat(n,1,xstar,"xstar");
  //  print_full_mat(n,n,A,"A");
  //  print_full_mat(n,1,xhat,"xhat0");
  solve_pivot(1, n, A, xhat, p, piv);
  //  print_full_mat(n,1,xhat,"xhat");
  //  fprintf(stdout,"\nSolution:\n");
  //  print_full_mat(n,1,xhat,"xhat");
  REAL xhatn=1e0,eps0=1e-10,xmax=1e0+eps0;
  /* check the solution if within bounds */
  INT flag = 0;
  for(j=0;j<n;j++){
    if((xhat[j] < -eps0) || (xhat[j] > xmax)){
      flag=(j+1);
      //      fprintf(stdout,"\nNOT FOUND: xhat(%d)=%e\n\n",flag,xhat[j]);
      break;
    }
    xhatn -= xhat[j];
    if((xhatn<-eps0) || (xhatn>xmax)) {
      flag=n+1;
      //          fprintf(stdout,"\nNOT FOUND: xhat(%d)=%e\n\n",flag,xhatn);
      break;
    }
  }
  if(A) free(A);
  if(xhat) free(xhat);
  if(p) free(p);
  if(piv) free(piv);
  return flag;
}
void marks(scomplex *sc,dvector *errors)
{
  /* mark simplices depending on the value of an estimator */
  /* the estimator here is the aspect ratio of the simplex */
  INT n=sc->n,n1=n+1,ns=sc->ns,level=sc->level;
  INT kbadel,ke,i,j,j1,k,p,ni,mj,mk;
  INT ne=(INT )((n*n1)/2);
  REAL slmin,slmax,asp,aspmax=-10.;;
  REAL *sl=(REAL *)calloc(ne,sizeof(REAL));
  INT kbad=0;
  for(i = 0;i<ns;i++){
    if(sc->gen[i] < level) continue;
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
unsigned int reflect2(INT n, INT is, INT it,				\
		      INT* sv1, INT *sv2, INT* stos1, INT* stos2,	\
		      INT visited, INT *wrk)
/*************************************************************/
/* it works for all meshes that can be consistently ordered. */
/********************************************************************/
{
  /* 
     sv1 are the n+1 vertices of is; sv2 are the (n+1) vertices of
     (it).  
     
     stos1 are the n+1 neighbors of is; stos2 are the (n+1) neighbors of
     (it).
     
     This routine checks whether is is reflected neighbor of it
     and reorders is if it was not visited before One main assumption is
     that (is) and (it) intersect in (n-1) dimensional simplex with (n)
     vertices, so that only one element of sv1 (say, k1) is not present
     in sv2 and only one element of sv2 (say, k2) is not present in
     sv1. Then we make an important assumption here, that stos1[k1] = it
     and stos[k2]= is. This is always achievable when creating the stos
     (simplex to simplex) relation.

     wrk is working space of size n+2, n is the spatial dimension 
  */
  INT n1=n+1,n2=n+2;
  INT *wrk1=NULL,*invp=NULL,*p=NULL,*pw=NULL,*wrk2=NULL;
  INT i,j;
  /**/
  /* we also check if we have reflected neighbors */
  if(visited){
    for (i=0; i<n1;i++){
      if(stos1[i]!=it){
	if(sv1[i]-sv2[i]) {
	  /* not reflected neighbors */
	  fprintf(stderr,"\n***ERROR in %s ; (is)=%d(vis=%d) and (it) = %d are both visited but are not reflected neighbors.\n\n",__FUNCTION__,is,visited,it);
	  fprintf(stderr,"\n***The problem is at node %d, (sv1=%d,sv2=%d)\n",i,sv1[i],sv2[i]);
	  return 2;
	}
      }
    }
    /* we have reflected neighbors, so we just return */
    return 0;
  }
  INT kv1=-1,kv2=-1;
  for (i=0; i<n1;i++){
    if(stos1[i] == it){
      kv1=sv1[i];
      break;
    }      
  }
  for (i=0; i<n1;i++){
    if(stos2[i] == is){
      kv2=sv2[i];
      break;
    }      
  }
  if (kv1<0 || kv2 < 0) {
    fprintf(stderr,"\n***ERROR in %s ; kv1=%d, kv2=%d must be positive.\n\n",__FUNCTION__,kv1,kv2);
    return 3;
  }
  wrk1=wrk; wrk2=wrk1+n2; p=wrk2+n2;invp=p+n2;  pw=invp+n2;
  memcpy(wrk1,sv1,n1*sizeof(INT));wrk1[n1] = kv2; 
  isi_sortp(n2,wrk1,p,pw);
  /*  returrn wrk1 to the initial state */
  memcpy(wrk1,sv1,n1*sizeof(INT));wrk1[n1] = kv2; 
  /* second array*/
  memcpy(wrk2,sv2,n1*sizeof(INT)); wrk2[n1] = kv1;
  isi_sortp(n2,wrk2,pw,invp);
  /* return wrk2 to init state */
  memcpy(wrk2,sv2,n1*sizeof(INT)); wrk2[n1] = kv1;
  /*
    We use here identity1: sv1[p1[k]] = sv2[p2[k]] for all k. Hence we have:
    
    s2[j] = s2[p2[invp2[j]]] = s1[p1[invp2[j]]]
    
    where we used the identity1 with k=invp2[j]
    
  */
  /* now we can use p2 and wrk1 to move around what we need */
  for (i=0; i<n1;i++){
    j=p[invp[i]]; /* this is the new index of sv1[i] */
    if(wrk2[i] != kv2){
      sv1[i] = wrk1[j];
    } else {
      sv1[i]=kv1;
    }
  }
  for (i=0; i<n1;i++){
    j=p[invp[i]]; /* this is the new index of sv1[i] */
    if(wrk2[i] != kv2){
      wrk1[i] = stos1[j];
    } else {
      wrk1[i] = it;
    }
  }
  for (i=0; i<n1;i++){
    stos1[i] = wrk1[i];
  }
  return 0;
}
/******************************************************************/
/*using bfs to get the reflected mesh*/
void abfstree(INT it, scomplex *sc,INT *wrk) 
{
  /* from the scomplex the mask are the marked and the */
  /* bfs tree rooted at simplex it */
  INT n=sc->n,n1=n+1,ns=sc->ns;
  INT i,j,k,iii,is,isn1,itn1;
  INT i1,in1,kbeg,kend,nums,iai,iai1,klev;
  INT *mask = sc->marked;
  INT *jbfs = sc->child0;
  INT ireflect=-10;
  // initialization
  /*************************************************************/
  INT *isnbr,*itnbr,*isv,*itv;
  /* caution: uses sc->marked as working array of size ns */
  memset(mask,0,ns*sizeof(INT));
  //  is=0;//(INT )(ns/2);
  nums=0;
  klev=1; //level number ; for indexing this should be klev-1;
  jbfs[nums]=it; // thit it an input simplex where to begin. 
  mask[it]=klev;
  nums++;
  kbeg=0; kend=1;
  while(1) {
    for(i1=kbeg;i1<kend;++i1){
      it=jbfs[i1];
      iai  = it*n1;
      iai1 = iai+n1;
      itn1 = it*n1;
      itnbr=(sc->nbr+itn1); 
      itv=(sc->nodes+itn1);
      for(k=iai;k<iai1;++k){
	is=sc->nbr[k];
	//	fprintf(stdout,"%i(nbr=%i) ",i,j);fflush(stdout);	  
	if(is<0) continue;
	isn1=is*n1;
	isnbr=(sc->nbr+isn1);
	isv=(sc->nodes+isn1);
	ireflect = reflect2(n,is,it,isv,itv,isnbr,itnbr,mask[is],wrk);
	switch(ireflect) {
	case 2 :
	case 3 :
	  exit(ireflect);
	  break;
	case 0 :
	  /* if it was visited and all is OK, we just return */
	  /* if(mask[it]) { */
	  /*   return 0; */
	  /* } else { */
	  /*   break; */
	  /* } */
	  break;
	default :
	  fprintf(stderr,						\
		  "Invalid return from reflect2 in %s; return value = %d\n", \
		  __FUNCTION__,ireflect);
	  exit(4);
	}
	if(!mask[is]){
	  jbfs[nums]=is;
	  mask[is]=klev;
	  //	  fprintf(stdout,"%i(%i,%i)",i,j,mask[j]);fflush(stdout);      
	  nums++;
	}
	
      }
    }
    kbeg=kend; kend=nums;klev++;
    if(nums >= ns)  break;
  }
  //  fprintf(stdout,"%%BFS levels for reflect: %d; ",klev-1);
  for(i=0;i<ns;i++){
    jbfs[i]=-1;
  }
  return;
}
void scfinalize(scomplex *sc)
{
  /* copy the final grid at position 1*/
  INT ns,j=-10,k=-10,n=sc->n,n1=sc->n+1;
  /*  
      store the finest mesh in sc structure. 
      on input sc has all the hierarchy, on return sc only has the final mesh. 
  */
  ns=0;
  for (j=0;j<sc->ns;j++){
    /*
      On the last grid are all simplices that were not refined, so
      these are the ones for which child0 and childn are not set. 
    */
    if(sc->child0[j]<0 || sc->childn[j]<0){
      for (k=0;k<n1;k++) {
	sc->nodes[ns*n1+k]=sc->nodes[j*n1+k];
	sc->nbr[ns*n1+k]=sc->nbr[j*n1+k];
      }
      sc->gen[ns]=sc->gen[j];
      sc->flags[ns]=sc->flags[j];
      ns++;
    }
  }
  sc->ns=ns;
  //  sc->nv=nv;
  fprintf(stdout,"\n%%After %d levels of refinement:\tsimplices=%d ; vertices=%d\n",sc->level,sc->ns,sc->nv); fflush(stdout);
  return;
}
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
