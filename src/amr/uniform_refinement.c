/*! \file src/amr/uniform_refinement.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 20170715.
 *  Copyright 2017__HAZMATH__. All rights reserved.
 *
 *  \note containing all essentials routines for uniform refinement in 2D and 3D. 
 *  Ludmil, Yuwen 20210604
 */
#include "hazmath.h"
/***********************************************************************************************/
/*!
 * \fn SHORT dcsr_sparse (dCSRmat *A, ivector *ii, ivector *jj, dvector *kk, INT m, INT n)
 *
 * \brief Form a dCSRmat A with m rows, n columns. Input vectors ii, jj, kk must be of equal lengths.
 *        For each index l, the (ii(l),jj(l))-entry of A is kk(l). In addition, if (ii(l_1),jj(l_1)),
 *        (ii(l_2),jj(l_2)),...,(ii(l_m),jj(l_m)) are identical, then the (ii(l_1),jj(l_1))-entry 
 *        of A is the sum of kk(l_1),kk(l_2),...,kk(l_m). 
 *        This function plays the same role as the function "sparse" in Matlab.
 * 
 * \param A   Pointer to dCSRmat matrix
 *
 * \return SUCCESS if successful; 
 *
 * \note Ludmil, Yuwen 20210606.
 */
SHORT dcsr_sparse (dCSRmat *A, ivector *ii, ivector *jj, dvector *kk, INT m, INT n)
{
      // get size
    INT nnz=kk->row;
    if ((ii->row!=jj->row)||(ii->row!=kk->row)){
      printf("The input vectors are not of equal length!");
      return 0;
    }
    dCOOmat A0;
    A0.row = m;A0.col = n;A0.nnz = nnz;
    A0.rowind = ii->val;A0.colind = jj->val;A0.val = kk->val;
    dcoo_2_dcsr(&A0,A);
    
    // copy pointers for easier reference
    INT *ia = A->IA;
    INT *ja = A->JA;
    REAL *a = A->val;
    INT i, ij,j,ih,iai;
    SHORT norepeat;
    INT maxdeg=ia[1]-ia[0];

    INT *ind = (INT *) calloc(n,sizeof(INT));
    for (i=0; i<n; ++i) ind[i]=-1;
    // clean up because there might be some repeated indices
    // compute max degree of all vertices (for memory allocation):
    for (i=1;i<m;++i){
      ih=ia[i+1]-ia[i];
      if(maxdeg<ih) maxdeg=ih;
    }
    REAL *atmp=calloc(maxdeg,sizeof(REAL));    
    INT *jatmp=calloc(maxdeg,sizeof(INT));    
    nnz=0;
    for (i=0;i<m;++i){
      // loop over each row. first find the length of the row:
      ih=ia[i+1]-ia[i];
      // copy the indices in tmp arrays.
      memcpy(jatmp,(ja+ia[i]),ih*sizeof(INT));
      memcpy(atmp,(a+ia[i]),ih*sizeof(REAL));
      norepeat=1;
      for(ij=0;ij<ih;++ij){
	j=jatmp[ij];
	if(ind[j]<0){
	  ind[j]=ij;
	} else {
	  norepeat=0; // we have a repeated index. 
	  atmp[ind[j]]+=atmp[ij];
	  jatmp[ij]=-abs(jatmp[ij]+1);
	}
      }
      for(ij=0;ij<ih;++ij){
	j=jatmp[ij];
	if(j<0) continue;// if j is negative, this has repeated somewhere. do nothing
	ind[j]=-1;// make, for all all visited j, ind[j]=-1;
      }
      if(norepeat) continue; // do nothing if no indices repeat.
      // put everything back, but now we have negative column indices
      // on the repeated column indices and we have accumulated the
      // values in the first position of j on every row. 
      memcpy(&ja[ia[i]],jatmp,ih*sizeof(INT));
      memcpy(&a[ia[i]],atmp,ih*sizeof(REAL));
    }
    if (ind) free(ind);
    if(atmp) free(atmp);
    if(jatmp) free(jatmp);
    // run over the matrix and remove all negative column indices.
    iai=ia[0];
    nnz=0;
    for (i=0;i<m;++i){
      for(ij=iai;ij<ia[i+1];++ij){
	j=ja[ij];
	if(j<0) continue;
	ja[nnz]=ja[ij];
	a[nnz]=a[ij];
	++nnz;
      }
      iai=ia[i+1];
      ia[i+1]=nnz;
    }
    A->nnz=nnz;
    A->val=realloc(A->val,A->nnz*sizeof(REAL));
    A->JA=realloc(A->JA,A->nnz*sizeof(INT));
    return SUCCESS;
}

/*****************************************************************************/
/*!
 * \fn void uniqueij(iCSRmat *U, ivector *ii, ivector *jj)
 *
 * \brief Form an iCSRmat U. Input column vectors ii, jj must be of
 *        equal lengths and be index vectors ranging from 0 to
 *        max(ii,jj).  It removes the duplicated (i,j)-pairs from the
 *        matrix in (ii,jj), rearrange the condensed (ii,jj) in
 *        ascending lexicographically order, and returns it in the
 *        icsr format U. This function is also used to obtain edge
 *        structures in a simplicial complax.
 * 
 * \param U   Pointer to iCSRmat matrix 
 *
 * \note  Ludmil, Yuwen 20210606.
 */
void uniqueij(iCSRmat *U, ivector *ii, ivector *jj)
{
    // transform to CSR format
    INT i, j, nr = ii->row, nv=-1;
    for (i=0;i<nr;i++) {
      if (nv<ii->val[i]) {nv=ii->val[i];}
      if (nv<jj->val[i]) {nv=jj->val[i];}
    }
    nv = nv + 1;
    INT nnz = ii->row;
    INT *ia = (INT *)calloc((nv+1),sizeof(INT));
    INT *ja = (INT *)calloc(nnz,sizeof(INT));

    INT *rowind = ii->val;
    INT *colind = jj->val;
    INT iind, jind;

    INT *ind0 = (INT *) calloc(nv+1,sizeof(INT));

    for (i=0; i<nnz; ++i) ind0[rowind[i]+1]++;

    ia[0] = 0;
    for (i=1; i<=nv; ++i) {
        ia[i] = ia[i-1]+ind0[i];
        ind0[i] = ia[i];
    }

    for (i=0; i<nnz; ++i) {
        iind = rowind[i];
        jind = ind0[iind];
        ja[jind] = colind[i];
        ind0[iind] = ++jind;
    }

    if (ind0) free(ind0);

    // delete duplicated entries
    INT ij,ih,iai;
    SHORT norepeat;
    INT maxdeg=ia[1]-ia[0];

    INT *ind = (INT *) calloc(nv,sizeof(INT));
    for (i=0; i<nv; ++i) ind[i]=-1;
    // clean up because there might be some repeated indices
    // compute max degree of all vertices (for memory allocation):
    for (i=1;i<nv;++i){
      ih=ia[i+1]-ia[i];
      if(maxdeg<ih) maxdeg=ih;
    }    
    INT *jatmp=calloc(maxdeg,sizeof(INT));    
    nnz=0;
    for (i=0;i<nv;++i){
      // loop over each row. first find the length of the row:
      ih=ia[i+1]-ia[i];
      // copy the indices in tmp arrays.
      memcpy(jatmp,(ja+ia[i]),ih*sizeof(INT));
      norepeat=1;
      for(ij=0;ij<ih;++ij){
      	j=jatmp[ij];
      	if(ind[j]<0){
      	  ind[j]=ij;
      	} else {
    	   norepeat=0; // we have a repeated index. 
	       jatmp[ij]=-abs(jatmp[ij]+1);
        	}
      }
      for(ij=0;ij<ih;++ij){
      	j=jatmp[ij];
	      if(j<0) continue;// if j is negative, this has repeated somewhere. do nothing
	      ind[j]=-1;// make, for all all visited j, ind[j]=-1;
      }
      if(norepeat) continue; // do nothing if no indices repeat.
      // put everything back, but now we have negative column indices
      // on the repeated column indices and we have accumulated the
      // values in the first position of j on every row. 
      memcpy(&ja[ia[i]],jatmp,ih*sizeof(INT));
    }
    if (ind) free(ind);
    if(jatmp) free(jatmp);
    // run over the matrix and remove all negative column indices.
    iai=ia[0];
    nnz=0;
    for (i=0;i<nv;++i){
      for(ij=iai;ij<ia[i+1];++ij){
      	j=ja[ij];
      	if(j<0) continue;
      	ja[nnz]=ja[ij];
	      ++nnz;
      }
      iai=ia[i+1];
      ia[i+1]=nnz;
    }
    ja=realloc(ja,nnz*sizeof(INT));
    U->row = nv; U->col = nv; U->nnz = nnz; U->IA = ia; U->val = NULL;
    //
    /* sorting the i-th row of U to get ja[ia[i]]:ja[ia[i+1]-1] in
       ascending lexicographic order */    
    iCSRmat UT;
    icsr_trans(U,&UT);
    icsr_trans(&UT,U);
    if(UT.IA) free(UT.IA);
    if(UT.JA) free(UT.JA);
    if(UT.val) free(UT.val);
    /**END sorting**/
    /**/
    /* if desired, use bubble sorting below: sorting the i-th row of U to get
       ja[ia[i]]:ja[ia[i+1]-1] in ascending lexicographic order */    
    //    INT tmp;
    /* for (i=0;i<nv;i++){ */
    /*   ih = ia[i+1]-ia[i]; */
    /*   for (j=0;j<ih;j++){ */
    /*     for (k=j+1;k<ih;k++) { */
    /*       if (ja[ia[i]+j]>ja[ia[i]+k]) { */
    /*         tmp = ja[ia[i]+j]; */
    /*         ja[ia[i]+j] = ja[ia[i]+k]; */
    /*         ja[ia[i]+k] = tmp; */
    /*       } */
    /*     } */
    /*   } */
    /* } */
    /* U->JA = ja;  */
    return;
}
/********************************************************************************/
/*!
 * \fn void get_edge2d(iCSRmat *e2v, iCSRmat *el2e, scomplex *sc)
 *
 * \brief Returns the edge-to-vertex matrix e2v and the
 *        element-to-edge vertex el2e in 2d.  The input sc is the
 *        pointer to the scomplex with element-to-vertex matrix and
 *        coordinates of grid vertices. e2v is an incidence matrix of
 *        ne by nv, where ne is the number of edges and nv the number
 *        of vertices. el2e is an incidence matrix of ns by ne, where
 *        ns is the number of simplexes. The (i,j)-entry of e2v is 1
 *        if the i-th edge contains the j-th vertex. The (i,j)-entry
 *        of el2e is 1 if the i-th simplex contains the j-th edge. The
 *        edges are ascendingly ordered in a global lexicographical
 *        way.  The order of elements for el2e->JA corresponding to
 *        the i-th row is given in local lexicographic order, (0, 1),
 *        (0, 2), (1, 2).
 * 
 * \param e2v  Pointer to iCSRmat edge-to-vertex matrix
 * \param el2e  Pointer to iCSRmat element-to-edge matrix
 *
 *  \note Yuwen 20210606.
 */
void get_edge2d(iCSRmat *e2v, iCSRmat *el2e, scomplex *sc)
{
  INT nv = sc->nv, ns = sc->ns, ns2=2*ns, ns3=3*ns, i, j, k, ih, n0, n1, n2, tmp;
  ivector ii = ivec_create(ns3), jj = ivec_create(ns3); 
  iCSRmat U, el2v_csr = icsr_create(ns,nv,ns3), v2e, el2e0;
  el2v_csr.IA[0] = 0;
  INT *el2v=sc->nodes;
  /* The pair  (ii,jj) encodes three edges in each element in ascending lexicographic order */
  for (k=0;k<ns;k++) {
    /* sorting the local three vertices in ascending order */
    n0 = 3*k; n1 = n0+1; n2 = n1+1;
    if (el2v[n0]>el2v[n1]) {tmp = n0; n0 = n1; n1 = tmp;}
    if (el2v[n0]>el2v[n2]) {tmp = n0; n0 = n2; n2 = tmp;}
    if (el2v[n1]>el2v[n2]) {tmp = n1; n1 = n2; n2 = tmp;}

    ii.val[k] = el2v[n0];jj.val[k] = el2v[n1];
    ii.val[k+ns] = el2v[n0];jj.val[k+ns] = el2v[n2];
    ii.val[k+ns2] = el2v[n1];jj.val[k+ns2] = el2v[n2];

    el2v_csr.IA[k+1] = el2v_csr.IA[k] + 3;
  }
  el2v_csr.JA = el2v;

  for (i=0;i<ns3;i++){el2v_csr.val[i]=1;}

  uniqueij(&U,&ii,&jj);
  ivec_free(&ii); ivec_free(&jj);
  
  /* Form the edge to vertex csr matrix */
  INT ne = U.nnz, counter = 0;
  e2v->row = ne;
  e2v->col = nv;
  e2v->nnz = 2*ne;
  
  INT *ia=U.IA, *ja=U.JA;
  e2v->JA = (INT*)malloc(2*ne*sizeof(INT));
  e2v->val = (INT*)malloc(2*ne*sizeof(INT)); 
  for (i=0;i<2*ne;i++){e2v->val[i]=1;}
  for (i=0;i<nv;i++) {
    ih = ia[i+1] - ia[i];
    for (j=0;j<ih;j++) {
      e2v->JA[counter+2*j] = i;
      e2v->JA[counter+2*j+1] = ja[ia[i]+j];
    }
    counter = counter + 2*ih;
  }

  e2v->IA = (INT*)malloc((ne+1)*sizeof(INT));
  e2v->IA[0] = 0;
  for (i=0;i<ne;i++) {
    e2v->IA[i+1] = e2v->IA[i] + 2;
  }
  icsr_free(&U);
  icsr_trans(e2v,&v2e);
  
  /* The i-th row of el2e0 enumerates all edges that shares at least one vertex with the i-th element*/
  icsr_mxm(&el2v_csr,&v2e,&el2e0);
  icsr_free(&v2e);
  free(el2v_csr.IA); free(el2v_csr.val);

  ia=el2e0.IA, ja=el2e0.JA; 
  INT *val=el2e0.val;
  
  /* The i-th row of el2e enumerates all edges that shares two vertices with the i-th element */
  el2e->row=ns; el2e->col=ne; el2e->nnz=3*ns;
  el2e->IA = (INT*)malloc((ns+1)*sizeof(INT));
  el2e->JA = (INT*)malloc(3*ns*sizeof(INT));
  el2e->val = NULL;
  el2e->IA[0] = 0; 
  counter = 0;
  for (i=0;i<ns;i++) {
    el2e->IA[i+1] = el2e->IA[i] + 3;
    ih=ia[i+1]-ia[i];
    for (j=0;j<ih;j++) {
      if (val[ia[i]+j]==2) {
        el2e->JA[counter] = ja[ia[i]+j];
        counter = counter + 1; 
      }
    }
  }
  
  /* sorting each row of el2e such that the order of three edges is locally ascending lexicographic*/
  INT i3, ej1, ej2, *jtmp=(INT*)calloc(3,sizeof(INT));
  for (i=0;i<ns;i++) {
    i3=3*i; 
    memcpy(jtmp,&el2e->JA[i3],3*sizeof(INT));
    for (j=0;j<3;j++) {
      ej1 = e2v->JA[2*jtmp[j]];
      ej2 = e2v->JA[2*jtmp[j]+1];
      if ( ((sc->nodes[i3]==ej1)&&(sc->nodes[i3+1]==ej2)) \
         || ((sc->nodes[i3]==ej2)&&(sc->nodes[i3+1]==ej1)) )
      {el2e->JA[i3] = jtmp[j];continue;}

      if ( ((sc->nodes[i3]==ej1)&&(sc->nodes[i3+2]==ej2)) \
         || ((sc->nodes[i3]==ej2)&&(sc->nodes[i3+2]==ej1)) )
      {el2e->JA[i3+1] = jtmp[j];continue;}

      if ( ((sc->nodes[i3+1]==ej1)&&(sc->nodes[i3+2]==ej2)) \
         || ((sc->nodes[i3+1]==ej2)&&(sc->nodes[i3+2]==ej1)) )
      {el2e->JA[i3+2] = jtmp[j];continue;}
    }
  }
  free(jtmp); 
  icsr_free(&el2e0); 
}
/*******************************************************************************************/
/*!
 * \fn void get_edge3d(iCSRmat *e2v, iCSRmat *el2e, scomplex *sc)
 *
 * \brief Returns the edge-to-vertex matrix e2v and the element-to-edge vertex el2e in 3d. 
 *        The input sc is the pointer to the scomplex with element-to-vertex matrix and 
 *        coordinates of grid vertices. e2v is an incidence matrix of ne by nv, where ne
 *        is the number of edges and nv the number of vertices. el2e is an incidence matrix 
 *        of ns by ne, where ns is the number of simplexes. The (i,j)-entry of e2v is 1 if 
 *        the i-th edge contains the j-th vertex. The (i,j)-entry of el2e is 1 if the i-th simplex
 *        contains the j-th edge. The edges are ascendingly ordered in a global lexicographical way.
 *        The order of elements for el2e->JA corresponding to the i-th row
 *        is given in local lexicographic order, (0, 1), (0, 2), (0, 3), (1, 2), (1, 3), (2, 3).
 * 
 * \param e2v  Pointer to iCSRmat edge-to-vertex matrix
 * \param el2e  Pointer to iCSRmat element-to-edge matrix
 *
 *  \note Yuwen 20210606.
 */
void get_edge3d(iCSRmat *e2v, iCSRmat *el2e, scomplex *sc)
{
  if (sc->n!=3) {printf("The dimension is not 3!\n");exit(1);}
  INT nv = sc->nv, ns = sc->ns, ns2=2*ns, ns3=3*ns, ns4=4*ns, ns5=5*ns, \
  i, j, k, ih, n0, n1, n2, n3, tmp;
  ivector ii = ivec_create(6*ns), jj = ivec_create(6*ns); 
  iCSRmat U, el2v_csr = icsr_create(ns,nv,4*ns), v2e, el2e0;
  INT *el2v = sc->nodes;
  el2v_csr.IA[0] = 0;
  /* The pair  (ii,jj) encodes 6 edges in each element in ascending lexicographic order */
  for (k=0;k<ns;k++) {
    n0 = 4*k; n1 = n0+1; n2 = n1+1; n3 = n2+1; 
    if (el2v[n0]>el2v[n1]) {tmp = n0; n0 = n1; n1 = tmp;}
    if (el2v[n0]>el2v[n2]) {tmp = n0; n0 = n2; n2 = tmp;}
    if (el2v[n0]>el2v[n3]) {tmp = n0; n0 = n3; n3 = tmp;}
    if (el2v[n1]>el2v[n2]) {tmp = n1; n1 = n2; n2 = tmp;}
    if (el2v[n1]>el2v[n3]) {tmp = n1; n1 = n3; n3 = tmp;}
    if (el2v[n2]>el2v[n3]) {tmp = n2; n2 = n3; n3 = tmp;}
    /*printf("%d\t%d\t%d\t%d\n",n0,n1,n2,n3);*/
    ii.val[k] = el2v[n0];jj.val[k] = el2v[n1];
    ii.val[k+ns] = el2v[n0];jj.val[k+ns] = el2v[n2];
    ii.val[k+ns2] = el2v[n0];jj.val[k+ns2] = el2v[n3];
    ii.val[k+ns3] = el2v[n1];jj.val[k+ns3] = el2v[n2];
    ii.val[k+ns4] = el2v[n1];jj.val[k+ns4] = el2v[n3];
    ii.val[k+ns5] = el2v[n2];jj.val[k+ns5] = el2v[n3];

    el2v_csr.IA[k+1] = el2v_csr.IA[k] + 4;
  }
  /*el2v_csr.JA = el2v;*/
  memcpy(el2v_csr.JA,el2v,4*ns*sizeof(INT));
  /*ivec_print(&ii);
  ivec_print(&jj);*/
  for (i=0;i<4*ns;i++){el2v_csr.val[i]=1;}
  /*icsr_print(&el2v_csr);*/
  uniqueij(&U, &ii, &jj);
  ivec_free(&ii);
  ivec_free(&jj);

  INT ne = U.nnz, counter = 0;
  e2v->row = ne;
  e2v->col = nv;
  e2v->nnz = 2*ne;
  /* form the edge to vertex csr matrix */
  INT *ia=U.IA, *ja=U.JA;
  e2v->JA = (INT*)calloc(2*ne,sizeof(INT));
  e2v->val = (INT*)calloc(2*ne,sizeof(INT)); /*memset(e2v->val,1,2*ne*sizeof(INT));*/
  for (i=0;i<2*ne;i++){e2v->val[i]=1;}
  for (i=0;i<nv;i++) {
    ih = ia[i+1] - ia[i];
    for (j=0;j<ih;j++) {
      e2v->JA[counter+2*j] = i;
      e2v->JA[counter+2*j+1] = ja[ia[i]+j];
    }
    counter = counter + 2*ih;
  }

  e2v->IA = (INT*)calloc((ne+1),sizeof(INT));
  e2v->IA[0] = 0;
  for (i=0;i<ne;i++) {
    e2v->IA[i+1] = e2v->IA[i] + 2;
  }
  icsr_free(&U);
  icsr_trans(e2v,&v2e);
  /* The i-th row of el2e0 enumerates all edges that shares at least one vertex with the i-th element*/
  icsr_mxm(&el2v_csr,&v2e,&el2e0);
  icsr_free(&v2e); 
  icsr_free(&el2v_csr); 

  ia=el2e0.IA, ja=el2e0.JA; 
  INT *val=el2e0.val;
  /* The i-th row of el2e enumerates all edges that shares two vertices with the i-th element*/
  el2e->row=ns; el2e->col=ne; el2e->nnz=6*ns;
  el2e->IA = (INT*)calloc((ns+1),sizeof(INT));
  el2e->JA = (INT*)calloc(6*ns,sizeof(INT));
  el2e->val = NULL;
  el2e->IA[0] = 0; 
  counter = 0;
  for (i=0;i<ns;i++) {
    el2e->IA[i+1] = el2e->IA[i] + 6;
    ih=ia[i+1]-ia[i];
    for (j=0;j<ih;j++) {
      if (val[ia[i]+j]==2) {
        el2e->JA[counter] = ja[ia[i]+j];
        counter = counter + 1; 
      }
    }
  }
  
  /* rearrange each row of el2e such that the local edges are ordered in local lexicographic order */
  INT i6, i4, ej1, ej2, *jtmp = (INT*)calloc(6,sizeof(INT));
  for (i=0;i<ns;i++) {
    i6=6*i; i4=4*i;
    memcpy(jtmp,&el2e->JA[i6],6*sizeof(INT));

    for (j=0;j<6;j++) {
      ej1 = e2v->JA[2*jtmp[j]];
      ej2 = e2v->JA[2*jtmp[j]+1];
      if ( ((sc->nodes[i4]==ej1)&&(sc->nodes[i4+1]==ej2)) \
         || ((sc->nodes[i4]==ej2)&&(sc->nodes[i4+1]==ej1)) )
      {el2e->JA[i6] = jtmp[j];continue;}

      if ( ((sc->nodes[i4]==ej1)&&(sc->nodes[i4+2]==ej2)) \
         || ((sc->nodes[i4]==ej2)&&(sc->nodes[i4+2]==ej1)) )
      {el2e->JA[i6+1] = jtmp[j];continue;}

      if ( ((sc->nodes[i4]==ej1)&&(sc->nodes[i4+3]==ej2)) \
         || ((sc->nodes[i4]==ej2)&&(sc->nodes[i4+3]==ej1)) )
      {el2e->JA[i6+2] = jtmp[j];continue;}

      if ( ((sc->nodes[i4+1]==ej1)&&(sc->nodes[i4+2]==ej2)) \
         || ((sc->nodes[i4+1]==ej2)&&(sc->nodes[i4+2]==ej1)) )
      {el2e->JA[i6+3] = jtmp[j];continue;}

      if ( ((sc->nodes[i4+1]==ej1)&&(sc->nodes[i4+3]==ej2)) \
         || ((sc->nodes[i4+1]==ej2)&&(sc->nodes[i4+3]==ej1)) )
      {el2e->JA[i6+4] = jtmp[j];continue;}

      if ( ((sc->nodes[i4+2]==ej1)&&(sc->nodes[i4+3]==ej2)) \
         || ((sc->nodes[i4+2]==ej2)&&(sc->nodes[i4+3]==ej1)) )
      {el2e->JA[i6+5] = jtmp[j];continue;}
    }
  }
  icsr_free(&el2e0);
  free(jtmp);
}

/*!
 * \fn void uniformrefine2d(scomplex *sc)
 *
 * \brief Returns a uniform refinement of the input 2d grid sc and update its contents,
 *        in particular the element-to-vertex matrix and coordinates of grid vertices. 
 *        This function simply addes new vertices at midpoints of each edge and divides 
 *        each triangle into four subtriangles. 
 * 
 * \param sc  Pointer to scomplex grid structure.
 *
 *  \note Yuwen 20210606.
 */
void uniformrefine2d(scomplex *sc)
{
  iCSRmat e2v, el2e;
  get_edge2d(&e2v,&el2e,sc);
  INT nv = sc->nv, nv2=nv*2, ns = sc->ns, ne=e2v.row, i, i12, i3,	\
   *el2v=(INT*)calloc(3*ns,sizeof(INT));
  memcpy(el2v,sc->nodes,3*ns*sizeof(INT));

  sc->ns=4*ns; sc->nv=nv+ne;
  sc->x = (REAL*)realloc(sc->x,2*(nv+ne)*sizeof(REAL));
  sc->nodes = (INT*)realloc(sc->nodes,3*4*ns*sizeof(INT));
  for (i=0;i<ne;i++) {
    sc->x[nv2+2*i] = ( sc->x[2*e2v.JA[2*i]]+sc->x[2*e2v.JA[2*i+1]] )/2;
    sc->x[nv2+2*i+1] = ( sc->x[2*e2v.JA[2*i]+1]+sc->x[2*e2v.JA[2*i+1]+1] )/2;
  }
  
  for (i=0;i<ns;i++) {
    i12=12*i; i3=3*i;

    sc->nodes[i12] = el2v[i3];
    sc->nodes[i12+1] = nv+el2e.JA[i3];sc->nodes[i12+2] = nv+el2e.JA[i3+1];
 
    sc->nodes[i12+3] = el2v[i3+1];
    sc->nodes[i12+4] = nv+el2e.JA[i3];sc->nodes[i12+5] = nv+el2e.JA[i3+2];

    sc->nodes[i12+6] = el2v[i3+2];
    sc->nodes[i12+7] = nv+el2e.JA[i3+1];sc->nodes[i12+8] = nv+el2e.JA[i3+2];

    sc->nodes[i12+9] = nv+el2e.JA[i3];
    sc->nodes[i12+10] = nv+el2e.JA[i3+1];sc->nodes[i12+11] = nv+el2e.JA[i3+2];
  }
  haz_scomplex_init_part(sc);
  icsr_free(&e2v);
  icsr_free(&el2e);
  free(el2v);
}

/***********************************************************************************************/
/*!
 * \fn void uniformrefine3d(scomplex *sc)
 *
 * \brief Returns a uniform refinement of the input 3d grid sc and update its contents,
 *        in particular the element-to-vertex matrix and coordinates of grid vertices. 
 *        This function is based on the paper 
 *        "J. Bey: Simplicial grid refinement: on Freudenthal's algorithm
 *         and the optimal number of congruence classes". It addes new vertices at midpoints 
 *        of all edges and divides each tetrahedron into eight smaller tetrahedra of equal volumes. 
 * 
 * \param sc  Pointer to scomplex grid structure.
 *
 * \note Yuwen 20210606.
 */
void uniformrefine3d(scomplex *sc)
{
  if (sc->n!=3) {printf("The dimension is not 3!\n");exit(1);}
  iCSRmat e2v, el2e;
  get_edge3d(&e2v,&el2e,sc);
  INT nv = sc->nv, nv3=nv*3, ns = sc->ns, ne=e2v.row, i, i32, i2, i3, i4, i6;
  // INT tmp,j;
  sc->ns=8*ns; sc->nv=nv+ne;
  sc->vols = (REAL*)realloc(sc->vols,sc->ns*sizeof(REAL));
  sc->x = (REAL*)realloc(sc->x,3*sc->nv*sizeof(REAL));
  for (i=0;i<ne;i++) {
    i3 = 3*i; i2 = 2*i;
    sc->x[nv3+i3] = ( sc->x[3*e2v.JA[i2]]+sc->x[3*e2v.JA[i2+1]] )/2;
    sc->x[nv3+i3+1] = ( sc->x[3*e2v.JA[i2]+1]+sc->x[3*e2v.JA[i2+1]+1] )/2;
    sc->x[nv3+i3+2] = ( sc->x[3*e2v.JA[i2]+2]+sc->x[3*e2v.JA[i2+1]+2] )/2;
    /*printf("%f\t%f\t%f\n",sc->x[nv3+i3],sc->x[nv3+i3+1],sc->x[nv3+i3+2]);*/
  }
  icsr_free(&e2v);

  INT *el2v=(INT*)calloc(4*ns,sizeof(INT));
  
  memcpy(el2v,sc->nodes,4*ns*sizeof(INT));
  free(sc->nodes);
  sc->nodes = (INT*)calloc(4*sc->ns,sizeof(INT));
  
  for (i=0;i<ns;i++) {
    i32 = 32*i; i6=6*i; i4=4*i;
    /* four sub-tetrahedra at four corners, local ordering is important*/
    sc->nodes[i32] = el2v[i4];sc->nodes[i32+1] = nv+el2e.JA[i6];
    sc->nodes[i32+2] = nv+el2e.JA[i6+1];sc->nodes[i32+3] = nv+el2e.JA[i6+2];
    
    sc->nodes[i32+4] = nv+el2e.JA[i6];sc->nodes[i32+5] = el2v[i4+1];
    sc->nodes[i32+6] = nv+el2e.JA[i6+3];sc->nodes[i32+7] = nv+el2e.JA[i6+4];

    sc->nodes[i32+8] = nv+el2e.JA[i6+1];sc->nodes[i32+9] = nv+el2e.JA[i6+3];
    sc->nodes[i32+10] = el2v[i4+2];sc->nodes[i32+11] = nv+el2e.JA[i6+5];

    sc->nodes[i32+12] = nv+el2e.JA[i6+2];sc->nodes[i32+13] = nv+el2e.JA[i6+4];
    sc->nodes[i32+14] = nv+el2e.JA[i6+5];sc->nodes[i32+15] = el2v[i4+3];
    /* four sub-tetrahedra from the inner octahedron, local ordering is important */ 
    sc->nodes[i32+16] = nv+el2e.JA[i6];sc->nodes[i32+17] = nv+el2e.JA[i6+1];
    sc->nodes[i32+18] = nv+el2e.JA[i6+2];sc->nodes[i32+19] = nv+el2e.JA[i6+4];
    
    sc->nodes[i32+20] = nv+el2e.JA[i6];sc->nodes[i32+21] = nv+el2e.JA[i6+1];
    sc->nodes[i32+22] = nv+el2e.JA[i6+3];sc->nodes[i32+23] = nv+el2e.JA[i6+4];

    sc->nodes[i32+24] = nv+el2e.JA[i6+1];sc->nodes[i32+25] = nv+el2e.JA[i6+2];
    sc->nodes[i32+26] = nv+el2e.JA[i6+4];sc->nodes[i32+27] = nv+el2e.JA[i6+5];

    sc->nodes[i32+28] = nv+el2e.JA[i6+1];sc->nodes[i32+29] = nv+el2e.JA[i6+3];
    sc->nodes[i32+30] = nv+el2e.JA[i6+4];sc->nodes[i32+31] = nv+el2e.JA[i6+5];
  }
  /*haz_scomplex_init_part(sc);*/

  icsr_free(&el2e);
  free(el2v);
} 
