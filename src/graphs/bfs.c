#include "hazmath.h"

typedef struct  
{
  REAL val;
  INT  id; //for the permutation
} weights;

typedef struct  
{
  INT mask;
  INT val;
  INT  id; //for the permutation
} iweights;

typedef INT (*testit)(const void*, const void*);

INT check0(weights *elem1, weights *elem2)
{
  if ( elem1->val < elem2->val)
    return -1;
   else if (elem1->val > elem2->val)
      return 1;
   else
      return 0;
}

INT check1(iweights *elem1, iweights *elem2)
{
  //descending order. 
  if  (elem1->mask > elem2->mask)
    return -1;
  else if(( elem1->mask == elem2->mask) && (elem1->val > elem2->val))
    return -1;
  else if(elem1->mask < elem2->mask)
    return 1;
  else if(( elem1->mask == elem2->mask) && (elem1->val < elem2->val))
    return 1;
  else
    return 0;
}

INT getp(INT *ie, INT *je, REAL *w, INT ne, INT *p)
{
  INT k;
  weights *tosort=NULL;  
  tosort = (weights *)calloc(ne,sizeof(weights));
  for (k=0; k<ne;k++)
    {
      tosort[k].val=w[k];
      tosort[k].id=k;
    }
  // print before
  /*
  fprintf(stdout,"\nNot Ordered:\n");
  for (k=0; k<ne;k++)
    fprintf(stdout, "id=%4i; value=%10.6f\n", tosort[k].id+1,tosort[k].val);
  */
  //Sort now.
  qsort((void *) tosort,ne,sizeof(weights),(testit )check0 );                  
  /* fprintf(stdout,"\nOrdered:\n");
  for (k=0; k<ne;k++)
    fprintf(stdout, "id=%d; value=%10.6f\n", tosort[k].id+1,tosort[k].val);
  fflush(stdout);
  */
  // after things are sorted,  we get the permutation
  for (k=0; k<ne;k++)
    p[k]=tosort[k].id;

  if(tosort) free(tosort);  
  //print again, this time ordered
  
  //  fprintf(stdout,"\nOrdered:\n");
  //  for (k=0; k<ne;k++)
  //    fprintf(stdout, "knew=%4i; idnew=%4i; value=%10.6f\n", k+1,p[k]+1,w[p[k]]);
  return 0;
}
INT getpz(REAL *z, INT nv, INT *p)
{
  INT k;
  weights *tosort=NULL;  
  tosort = (weights *)calloc(nv,sizeof(weights));
  for (k=0; k<nv;k++)
    {
      tosort[k].val=-z[k];
      tosort[k].id=k;
    }
  qsort((void *) tosort,nv,sizeof(weights),(testit )check0 );                  
  // after things are sorted,  we get the permutation
  for (k=0; k<nv;k++)
    p[k]=tosort[k].id;
  if(tosort) free(tosort);  
  return 0;
}
INT getpi(INT *iz, INT *maskv, INT nv, INT *p)
{
  INT k;
  iweights *tosort=NULL;  
  tosort = (iweights *)calloc(nv,sizeof(iweights));
  for (k=0; k<nv;k++)
    {
      tosort[k].mask=maskv[k];
      tosort[k].val=iz[k];
      tosort[k].id=k;
    }
  qsort((void *) tosort,nv,sizeof(iweights),(testit )check1 );                  
  // after things are sorted,  we get the permutation
  for (k=0; k<nv;k++)
    p[k]=tosort[k].id;
  if(tosort) free(tosort);  
  return 0;
}

void bfs(INT nv, INT *ia, INT *ja, INT *ibfs, INT *jbfs,	\
	INT *maske, INT *p, INT *et, INT *lev, REAL *w,REAL *z)
{
  INT i,j,k;
  INT i1,i2,k0,mj,kbeg,kend,ipoint,iai,iai1,klev;
  // intialize  
  memset(maske,0,nv*sizeof(INT));
  klev=1; //level number ; for indexing this should be klev-1;
  kbeg=ibfs[klev-1];
  kend=ibfs[klev];
  for(i1=kbeg;i1<kend;++i1){
    i=jbfs[i1];
    maske[i]=klev;//;
    et[i]=-1;
    p[i1-kbeg]=i1-kbeg;//trivial permutation.
  }
  //  fprintf(stdout,"level=%i; number of vertices: %i\n",klev,kend-kbeg);
  ipoint=ibfs[1];
  while(1) {
    k0=0;
    for(i2=kbeg;i2<kend;++i2){
      i1=p[i2-kbeg]+kbeg;
      i=jbfs[i1];
      iai = ia[i];
      iai1=ia[i+1];     
      for(k=iai;k<iai1;++k){
	j=ja[k];
	if(i==j) continue;
	mj=maske[j];
	if(!mj){
	  jbfs[ipoint]=j;
	  maske[j]=klev;
	  // bfs tree edge
	  et[j]=i; //father or mother of j is i.
	  w[k0]=z[j];
	  k0++;
	  ipoint++;
	}
      }	   
    }
    kbeg=kend;
    klev++;
    ibfs[klev]=ipoint;
    kend=ipoint;
    //    fprintf(stdout,"level=%i; number of vertices: %i\n",klev,kend-kbeg);
    //    fprintf(stdout,"level=%i; number of vertices: %i : %i\n",klev,kend-kbeg,k0);
    getpz(w, k0, p);
    if(ipoint >=nv)  break;
  }
  //  fprintf(stdout,"level=%i; TOTAL number of vertices: %i and %i\n",klev,ibfs[klev],nv);
  *lev=klev;
return;
}

void bfstree(INT root, INT nv, INT ne, INT *ia, INT *ja, INT *ibfs, INT *jbfs, \
	    INT *mask, INT *et, INT *lev, INT *ledge, REAL *w,REAL *wo) 
{
  /* bfs tree rooted at root */
  INT i,j,k,maxdeg,ih;
  INT i1,kbeg,kend,ipoint,iai,iai1,klev;
  if(root < 0) {
    maxdeg=-1;
    for(i=0;i<nv;++i){
      ih=ia[i+1]-ia[i];
      if(maxdeg<ih){
	maxdeg=ih;
	root=i;
      }
    }
    if(root<0){
      fprintf(stderr,"ERROR: could not find root for bfs\n");
      exit(129);
    }
  }
  // initialization
  /*************************************************************/
  memset(mask,0,nv*sizeof(INT));
  for(i=0;i<nv;++i) et[i]=-1;
  klev=1; //level number ; for indexing this should be klev-1;
  ibfs[0]=0;
  ibfs[1]=1;
  jbfs[ibfs[0]]=root;
  mask[root]=klev;//;
  wo[jbfs[ibfs[0]]]=-1.;// there is no edge associated with the root of the tree. 
  fprintf(stdout,"ia,ja %i:  %i, root=%i\n",ia[root],ia[root+1],root+1);
  ipoint=ibfs[1];
  kbeg=ibfs[0];
  kend=ibfs[1];
  while(1) {
    for(i1=kbeg;i1<kend;++i1){
      i=jbfs[i1];
      iai = ia[i];
      iai1=ia[i+1];
      ih=iai1-iai-1;
      if(!ih) continue;
      for(k=iai;k<iai1;++k){
	j=ja[k];
	//	fprintf(stdout,"%i : %i\n",j, ia[j+1]-ia[j]);
	if(i==j) continue;
	if(!mask[j]){
	  jbfs[ipoint]=j;
	  mask[j]=klev;
	  // bfs tree edge
	  et[j]=i; //parent of j is i.
	  wo[ipoint]=w[ledge[k]];
	  ipoint++;
	}
      }	   
    }
    //    fprintf(stdout,"level=%i; number of vertices: %i;
    //    test=%i\n", klev,kend-kbeg,ipoint-kend);
    if(ipoint==ibfs[klev]) break;
    kbeg=kend;
    klev++;
    ibfs[klev]=ipoint;
    kend=ipoint;
    if(ipoint>=nv)  break;
  }
  //  fprintf(stdout, "\nA1=[");
  //  for(i=0;i<ipoint;i++){
  //    j=jbfs[i];
  //    if(et[j]<0) continue;
    //        fprintf(stdout,
    //    	    "leaf: %i, node=%i with mask %i;  parent=%i;, weight=%12.4e\n",
    //    	    i+1,j,mask[j],et[j],wo[i]);
    //    fprintf(stdout, "%7i %7i %16.8e\n",j+1,et[j]+1,wo[i]);
    //fprintf(stdout, "%7i %7i %16.8e\n",et[j]+1,j+1,wo[i]);
    //  }
  //   fprintf(stdout, "];\n");
  *lev=klev;
  return;
}
  /* for(i=0;i<nv;i++){ */
  /*   iai=ia[i];iai1=ia[i+1]; */
  /*   for(k=iai;k<iai1;k++){ */
  /*     j=ja[k]; */
  /*     if(i==j)continue; */
  /*     fprintf(stdout, "%7i %7i %16.8e\n",i+1,j+1,w[ledge[k]]); */
  /*   } */
  /* } */
  /* fprintf(stdout, "];\n"); */
