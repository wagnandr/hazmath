/************************************************************************/
#include "hazmath.h"
scomplex *umesh(const INT dim,		\
		INT *nd, cube2simp *c2s,		\
		INT *isbndf, INT *codef,INT elflag,	\
		const INT intype)
{
  /* face is the face that matches the face_parent in the neighboring
     element.  uniform simplicial mesh of the unit cube in dimension
     dim.  dim is the dimension, nd is the number of grid points in
     each dimension.  ordering is lexicographically by
     name=(x[0],...,x[n]).  more than 3D is not fully tested xmacro[]
     are the coordinates of a domain isomorphic to the cube via a
     bilinear or "Q2" change of coordinates. output is a simplicial
     complex sc.

     if(intype == -2) use unirefine() function (from unigrid.c in src/amr)

     if(intype == -1)construct grid using diagonals pointing
     0-7(0...0)-->(1...1).

     if (intype>0) starting with intype the mesh is constructed like
     criss-cross grid. This works in 2D and 3D, and is unclear whether
     it works in d>3.
  */
  INT iz1;
  INT jperm,i,j,flag,kf,type;
  INT dim1 = dim+1;
  // m is dim+1 so that we can handle even dimensions
  INT *m = (INT *)calloc(dim1,sizeof(INT));
  INT *mm = (INT *)calloc(dim1,sizeof(INT));
  INT *cnodes = (INT *)calloc(c2s->nvcube,sizeof(INT));  
  //  INT *icycle = (INT *)calloc(dim+1,sizeof(INT));
  INT nv=1,ns=1;
  for(i=0;i<dim;i++){
    nv*=(nd[i]+1);
    ns*=nd[i];
  }
  ns*=c2s->ns; /*multiply by the number of simplices in the unit cube
		 (2 in 2D and 6 in 3d and 24 in 4d*/
  scomplex *sc = (scomplex *)haz_scomplex_init(dim,ns,nv);
  //  fprintf(stdout,"\nFaces=(%d,%d)=(face,face_parent)\n",face,face_parent);fflush(stdout);
  for(kf=0;kf<sc->nv;kf++){
    coord_lattice(m,dim,kf,sc->nv,nd);
    for(i=0;i<dim;i++){
      /* THIS HERE HAS THE X COORD FIRST WHICH IS NOT WHAT ONE HAS IF
	 USING THE BIJECTION BETWEEN BINARY NUMBERS AND THE
	 COORDINATES OF VERTICES IN THE UNIT CUBE> SO WE REVERSE THE
	 ORDERING OF DIVISIONS SO THAT WE PARTITION FIRST X and so
	 on. so basically we come here with the last coordinate
	 first. That is why we also have (dim-i-1) instaed of i*/
      sc->x[kf*dim+(dim-i-1)]=((REAL )m[i])/((REAL )nd[i]);
      /* OLD: sc->x[kf*dim+i]=((REAL )m[i])/((REAL )nd[i]); */
    }    
    //      print_full_mat_int(1,dim,m,"m1=");
    //      fprintf(stdout,"; iglobal=%d;",kf);
  }
  ns=0;
  for(kf=0;kf<sc->nv;kf++){
    coord_lattice(m,dim,kf,sc->nv,nd);
    flag=0;
    for(i=0;i<dim;i++){
      if(m[i]==nd[i]) {flag=1; break;}
    }
    if(flag) continue;
    if(intype==-1) {
      type=0;
    }else{
      /*criss-cross in any D*/
      /* determine type; this is not fully rigorously justified, but
       works in d=2,3*/
      type=(m[0]+intype)%2;
      for(i=1;i<dim-1;i++){
	type+=2*(m[i]%2);
      }
      // this is a hack here to work in 2D. unclear how to do in 2D yet or 4D. 
      if(dim==2){type=(abs(m[1]-m[0])+intype)%dim;}
      if((m[dim-1]%2)) type=dim-type;
      if((!(dim%2)) && (type>=(dim))) {type%=(dim);}
      if(dim%2 && (type>(dim+1))) {type%=(dim+1);}
    }
    //    type=0;
    /*depending on the type, split a cube in simplices*/
    //    fprintf(stdout,"\ntype=%d\n",type+1);
    for(j=0;j<c2s->nvcube;j++){
      //            fprintf(stdout,"j:%d; ",j+1);
      for(i=0;i<dim;i++){
    	mm[i]=m[i]+(c2s->bits[dim*j+i]);
	//		fprintf(stdout,"mm[%d]=%d; ",i+1,mm[i]+1);
      }
      cnodes[j]=num_lattice(mm,dim,nd);
      //            fprintf(stdout,"\n"); fflush(stdout);
    }   
    //    fprintf(stdout,"\n"); fflush(stdout);
    //  
    for(i=0;i<c2s->ns;i++){
      for(j=0;j<dim1;j++){
	iz1=c2s->nodes[i*dim1+j];
	jperm=c2s->perms[type*c2s->nvcube+iz1];
	//	fprintf(stdout,"\ntype=%d,ns=%d,jperm=%d,iz1=%d",type,ns,jperm,iz1);
	sc->nodes[ns*dim1+j]=cnodes[jperm];
      }
      sc->flags[ns]=elflag;
      ns++;      
    }    
  }
  INT cfbig=((INT )MARKER_BOUNDARY_NO)+100;
  INT facei,bf,cf,mi;
  //  INT kfp,ijk,mi,mip,toskip,toadd;
  /******************************************************************/
  /*  
   *  when we come here, all boundary faces have codes and they are
   *  non-zero. All interior faces shoudl have a code zero.
   */
  /******************************************************************/
  for(kf=0;kf<sc->nv;kf++) sc->bndry[kf]=cfbig;
  /* for(facei=0;facei<c2s->nf;facei++){ */
  /*   if(facei<dim){ */
  /*     mi=dim-(facei+1); */
  /*     bf=0; */
  /*   } else{ */
  /*     mi=dim-((facei%dim)+1); */
  /*     bf=nd[mi]; */
  /*   } */
  /*   cf=codef[facei]; */
  /*   if(!isbndf[facei]){ */
  /*     // first pass: set the interior faces; */
  /*     for(kf=0;kf<sc->nv;kf++){ */
  /* 	coord_lattice(m,dim,kf,sc->nv,nd); */
  /* 	if(m[mi]==bf){ */
  /* 	  if(sc->bndry[kf]>cf && (cf !=0)) sc->bndry[kf]=cf; */
  /* 	} */
  /*     } */
  /*   } */
  /* } */
  /******************************************************************/
  // second pass: set boundaries, so that the bondaries are the ones
  // that we care about:
  /******************************************************************/
  for(facei=0;facei<c2s->nf;facei++){
    if(facei<dim){
      mi=dim-(facei+1);
      bf=0;
    } else{
      mi=dim-((facei%dim)+1);
      bf=nd[mi];
    }
    cf=codef[facei];
    /* INT isbf=isbndf[facei]; */
    if(isbndf[facei]){
      for(kf=0;kf<sc->nv;kf++){
	coord_lattice(m,dim,kf,sc->nv,nd);
	if(m[mi]==bf){
	  if(sc->bndry[kf]>cf) sc->bndry[kf]=cf;
	}
      } 
    }
  }
  /******************************************************************/
  // Only interior points should now be left; set them to 0:
  for(kf=0;kf<sc->nv;kf++)
    if(sc->bndry[kf]>=cfbig) sc->bndry[kf]=0;      
  /******************************************************************/
  return sc;
}
/**************************************************************************/
void unirefine(INT *nd,scomplex *sc)  
{
/* 
 * refine uniformly l levels, where 2^l>max_m nd[m] using the generic
 * algorithm for refinement.  Works in the following way: first
 * construct a grid with refinements up to 2^{l} such that 2^{l}>max_m
 * nd[m]. then remove all x such that x[k]>nd[k]*2^{-l} and then remap
 * to the unit square.
 * (20180718)--ltz
*/
  INT ndmax=-1,i=-1,j=-1;
  for(i=0;i<sc->n;i++)
    if(ndmax<nd[i]) ndmax=nd[i];
  fprintf(stdout,"\nmax split=%d",ndmax);
  REAL sref=log2((REAL )ndmax);
  if(sref-floor(sref)<1e-3)
    sref=floor(sref);
  else
    sref=floor(sref)+1.;
  INT ref_levels= sc->n*((INT )sref);
  fprintf(stdout,"\nlog2 of the max=%e, l=%d",log2((REAL )ndmax)+1,ref_levels);
  ref_levels=0;
  if(ref_levels<=0) return;
  INT nsold;//ns,nvold,level;
  if(!sc->level){
    /* form neighboring list; */
    find_nbr(sc->ns,sc->nv,sc->n,sc->nodes,sc->nbr);
    //    haz_scomplex_print(sc,0,__FUNCTION__);  fflush(stdout);
    INT *wrk=calloc(5*(sc->n+2),sizeof(INT));
    /* construct bfs tree for the dual graph */
    abfstree(0,sc,wrk);
    //    haz_scomplex_print(sc,0,__FUNCTION__);fflush(stdout);
    //    exit(100);
    if(wrk) free(wrk);
  }
  //INT n=sc->n, n1=n+1,level=0;
  fprintf(stdout,"refine: ");
  while(sc->level < ref_levels && TRUE){
    nsold=sc->ns;
    //    nvold=sc->nv;
    for(j = 0;j < nsold;j++)sc->marked[j]=TRUE;
    for(j = 0;j < nsold;j++)
      if(sc->marked[j] && (sc->child0[j]<0||sc->childn[j]<0))
	haz_refine_simplex(sc, j, -1);
    /* new mesh */
    //    ns=sc->ns; 
    sc->level++;
    fprintf(stdout,"u%du",sc->level);//,nsold,ns,nv);
  }
  fprintf(stdout,"\n");
  scfinalize(sc);
  return;
}

