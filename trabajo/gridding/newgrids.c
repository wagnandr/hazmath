#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <getopt.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "hazmath.h"
#include "grid_defs.h"
#ifndef INT
#define INT int
#endif
#ifndef REAL
#define REAL double
#endif
/********************************FINCTIONS:*********************/
void cube2simp_free(cube2simp *c2s);
INT reverse(void *arr,INT length, size_t elsize);
cube2simp *cube2simplex(INT dim);
scomplex *umesh(const INT dim, INT *nd, cube2simp *c2s, const INT intype);
void polar2cart(INT dim, REAL *px, REAL *cx);
REAL interp8(cube2simp *c2s, REAL *u, REAL *ue, REAL *xhat);
REAL interp4(cube2simp *c2s, REAL *u, REAL *xhat);
void unirefine(INT *nd,scomplex *sc);
/***********************************************************************/
scomplex *macro_split(input_grid *g0,cube2simp *c2s)
{
  /* 
     from an input grid read from a file, creates an array of input
     grids each having a single macroelement
  */
  input_grid *g;
  scomplex *sc;
  INT i,j0,j1,k,nel0=g0->nel,nvcube=c2s->nvcube,nvface=c2s->nvcube;
  g=malloc(1*sizeof(input_grid *));
  g->ncsys=g0->ncsys;
  g->ox=g0->ox;
  g->systypes=g0->systypes;
  g->syslabels=g0->syslabels;
  g->nv=c2s->nvcube;
  g->nf=c2s->nvface;
  g->ne=c2s->ne;
  g->mnodes=(INT *)calloc(g->nel*(nvcube+1),sizeof(INT));
  memcpy(g->mndodes,g0->mnodes(

  for(i=0;i<c2s->ne;i++){
    k=
    j0=c2s->edges[2*i];
    j1=c2s->edges[2*i+1];
  }
  g->edges=c2s->edges;
  
  g->xv=(REAL *)calloc(g->dim*g->nv,sizeof(REAL)); 
  g->xe=(REAL *)calloc(g->dim*g->ne,sizeof(REAL)); 
  g->seg=(INT *)calloc(3*g->ne,sizeof(INT));
  
  for(i=0;i<nel0;i++){
    // get the macroelement:
  }
  return sc;  
}
/***********************************************************************/
INT *set_input_grid1(cube2simp *c2s,input_grid *g)
{
  /* 
     Every edge is put into a subset, i.e. two edges (i1,i2) and (j1,j2)
     are considered equivalent iff (i2-i1)=(j2-j1).  The number of
     divisions in an equivalent set of edges is taken to be the
     largest from the equivalence class.  OUTPUT array is a "dim"
     array and for each direction gives the number of partitions.
  */
  INT i,j,k,iri,ici,pmem;
  pmem=2*g->nv;
  if(pmem<g->ne) pmem=2*g->ne;
  if(pmem<g->nel) pmem=2*g->nel;
  if(pmem<g->nf) pmem=2*g->nf;
  INT *p=calloc(pmem,sizeof(INT));// permutation and inverse permutation;
  //
  for (i=0;i<g->ne;i++){
    iri=g->seg[3*i];
    ici=g->seg[3*i+1];
    if(iri<ici){
      g->seg[3*i]=iri;
      g->seg[3*i+1]=ici;
    } else {
      g->seg[3*i]=ici;
      g->seg[3*i+1]=iri;
    }
    /* set up divisions */
    j=g->seg[3*i+1]-g->seg[3*i]; // should be always positive;
    if(g->seg[3*i+2]>p[j])
      p[j]=g->seg[3*i+2];
  } 
  for (i=0;i<g->ne;i++){
    j=g->seg[3*i+1]-g->seg[3*i];
    g->seg[3*i+2]=p[j]; 
    //    fprintf(stdout,"\n[%d,%d]:div=%d",g->seg[3*i],g->seg[3*i+1],g->seg[3*i+2]);
  }
  for (i=0;i<g->ne;i++){
    j=g->seg[3*i+1]-g->seg[3*i];
    g->seg[3*i+2]=p[j]; 
    //    fprintf(stdout,"\n[%d,%d]:div=%d",g->seg[3*i],g->seg[3*i+1],g->seg[3*i+2]);
  }
  /*ORDER*/
  ilexsort(g->ne, 3,g->seg,p);
  ilexsort(g->nel,(c2s->nvcube+1),g->mnodes,p);
  ilexsort(g->nf, (c2s->nvface+1),g->mfaces,p);
  k=0;
  for (i=0;i<g->ne;i++){
    if(g->seg[3*i]) continue;
    j=g->seg[3*i+1]-g->seg[3*i]-1;
    p[k]=g->seg[3*i+2];
    k++;
    fprintf(stdout,"\n[%d,%d]:div=%d",g->seg[3*i],g->seg[3*i+1],g->seg[3*i+2]);
  }
  p=realloc(p,g->dim*sizeof(INT)); // realloc to dimension g->dim
  for (i=0;i<g->dim;i++){
    fprintf(stdout,"\ndirection:%d; div=%d",i,p[i]);
  }
  //  input_grid_print(g);
  print_full_mat_int(g->ne,3,g->seg,"med");
  print_full_mat_int(g->nf,(c2s->nvface+1),g->mfaces,"mf");
  print_full_mat_int(g->nel,(c2s->nvcube+1),g->mnodes,"mel");
  return p;
}
/*********************************************************************/
void map2mac(scomplex *sc,cube2simp *c2s, input_grid *g)
{
  /* 
     maps a uniform grid from the n-dimensional cube to a hexagonal 
     macroelement given by its coordinates xmacro[1:nvcube*dim]
     xmac[nvcube][dim]
  */
  INT i,j,k1,k2,k1c,k2c,kf,dim=sc->n;
  INT ksys;
  REAL *xmac=g->xv;  
  REAL *xhat = (REAL *)calloc(dim,sizeof(REAL));
  REAL *xemac=(REAL *)calloc(c2s->ne*dim,sizeof(REAL));
  // convert midpoints from polar to cartesian.
  for(i=0;i<c2s->ne;i++){
    k1=c2s->edges[2*i];
    k2=c2s->edges[2*i+1];
    k1c=g->systypes[g->csysv[k1]];
    k2c=g->systypes[g->csysv[k2]];
    fprintf(stdout,"\nverts=(%d,%d); coord_sys=(%d,%d)",k1,k2,k1c,k2c);fflush(stdout);
    if(g->csysv[k1]==g->csysv[k2] && k1c==1){
      //use xhat as a temp array:
      xhat[0]=0.5*(xmac[k1*dim]+xmac[k2*dim]);// this is rho
      // take half angles;
      for(j=1;j<dim;j++) {
	xhat[j]=0.5*(xmac[k1*dim+j]+xmac[k2*dim+j]);
      }
      polar2cart(dim,xhat,xemac+(i*dim));
      // translate by adding the origin. 
      ksys=g->csysv[k1];// k1c and k2c should be the same below. 
			      //      k2c=g->csysv[k2];
      for(j=0;j<dim;j++) {
	xemac[i*dim+j]+=g->ox[ksys*dim+j];
      }
    }
  }
  // end of mid points in polar;
  // now convert all vertices in cartesian as well. 
  for(i=0;i<c2s->nvcube;i++){
    k1c=g->systypes[g->csysv[i]];
    if(k1c==1){
      memcpy(xhat,xmac+i*dim,dim*sizeof(REAL));
      polar2cart(dim,xhat,xmac+(i*dim));
      //      translate
    }
    ksys=g->csysv[i];
    for(j=0;j<dim;j++) {
      xmac[i*dim+j]+=g->ox[ksys*dim+j];
    }
  }
  // now everything is in cartesian, so midpoints that are
  // not yet attended to are just averages.
  for(i=0;i<c2s->ne;i++){
    k1=c2s->edges[2*i];
    k2=c2s->edges[2*i+1];
    k1c=g->systypes[g->csysv[k1]];
    k2c=g->systypes[g->csysv[k2]];
    //skip all polar mid points
    if(g->csysv[k1]==g->csysv[k2] && k1c==1) continue;
    fprintf(stdout,"\ncart:verts=(%d,%d); coord_sys=(%d,%d)",k1,k2,k1c,k2c);fflush(stdout);
    for(j=0;j<dim;j++) {
      xemac[i*dim+j]=0.5*(xmac[k1*dim+j]+xmac[k2*dim+j]);
    }
  }
  r2c(c2s->nvcube,dim,sizeof(REAL),xmac); // we need xmac by rows here
  r2c(c2s->ne,dim,sizeof(REAL),xemac); // we need xemac (mid points of
				       // edges) also by rows
  for(kf=0;kf<sc->nv;kf++){
    for(i=0;i<dim;i++)xhat[i]=sc->x[kf*dim+i];
    for(i=0;i<dim;i++){
      //      fprintf(stdout,"coordinate:%d",i);
      sc->x[kf*dim+i]=interp8(c2s,xmac+i*c2s->nvcube,xemac+i*c2s->ne,xhat);
      //      sc->x[kf*dim+i]=interp4(c2s,xmac+i*c2s->nvcube,xhat);
    }
  }
  //  r2c(dim,c2s->nvcube,sizeof(REAL),xmac); // we need xmac by columns here
  //  r2c(dim,c2s->ne,sizeof(REAL),xemac); // we need xemac by rows agin
  if(xhat) free(xhat);
  if(xemac) free(xemac);
  return;
}
INT main(INT argc, char **argv)
{
  //  INT j=-1;
  input_grid *g=parse_input_grid("grid.input");
  INT dim=g->dim;
  cube2simp *c2s=cube2simplex(dim);
  /*------------------------------------------------------*/
  scomplex *sc;
  INT intype=0;
  /*------------------------------------------------------*/
  INT *nd=set_input_grid1(c2s,g);  
  /*this can be used to generate grids in a different way, but not now:*/
  /* if(intype<-1){ */
  /*   for(i=0;i<dim;i++) ndd[i]=1; */
  /*   sc=umesh(dim,ndd,c2s,intype); */
  /*   unirefine(nd,sc);     */
  /* }else{ */
  /*   sc=umesh(dim,nd,c2s,intype); */
  /* } */
  /*GENERATE UNIFORM GRID: nodes in each direction: ND*/
  exit(22);
  sc=umesh(dim,nd,c2s,intype);
  fprintf(stdout,"\nGenerated a uniform mesh in dim=%d; vertices: %d, simplexes %d",dim,sc->nv,sc->ns);
  if(nd) free(nd);
  fprintf(stdout,"\nedges=%d",c2s->ne);
  //  INT k1,k2,j1,j2,l1,l2;
  //KEEP KEEP  gcomplex *macs=form_macro(g);
  fprintf(stdout,"\nMapping back to the macroelement...\n");
  map2mac(sc,c2s,g);
  input_grid_free(g); 
  fprintf(stdout,"\nDone.\n");
  //  haz_scomplex_print(sc,0,"HAHA");
  if(dim==2||dim==3) {
    fprintf(stdout,"Writing vtk file...\n");
    vtkw("newmesh.vtu",sc,0,0,1.);
  }
  cube2simp_free(c2s);
  haz_scomplex_free(sc);
  return 0;
}
/*********************EOF**********************************/
