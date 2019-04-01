/*! \file src/utilities/solve_full.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 5/13/15.
 *  Copyright 2016__HAZMATH__. All rights reserved.
 *  
 *  \note: modified by Xiaozhe Hu on 10/25/2016
 *  \note: modified on 10/12/2017 (ltz1)
 *
 *  \note routines to solve Ax=b and calculate inv(A). 
 *
 */
#include "hazmath.h"
/*******************************************************************/
/* \fn  void c2r(const INT n, const INT m, const size_t sizeel, void *x)
 *
 * \note: converts 2d array stored by columns to a 2d array stored by
 *     rows (fortran to c); overwrites the input array x with the
 *     result. sizeel is the size of an element in the array in bytes.
 *
*/
void c2r(const INT n, const INT m, const size_t sizeel, void *x)
{
  INT i,j,ij,ji,nms=n*m*sizeel;
  void *y=(void *)malloc(nms);
  memcpy(y,x,nms);
  for (i=0;i<n;i++){
    for (j=0;j<m;j++){
      ji=sizeel*(n*j+i);
      memcpy(x,(y+ji),sizeel);
      x+=sizeel;
    }
  }
  if(y) free(y);
  return;
}
/*******************************************************************/
/* \fn void r2c(const INT n, const INT m, const size_t sizeel, void *x)
 *
 * \note: converts 2d array stored by rows to a 2d array stored by
 *        columns (c to fortran). sizeel is the size of an element in
 *        the array in bytes
 *
*/
void r2c(const INT n, const INT m, const size_t sizeel, void *x)
{
  INT i,j,ij,ji,nms=n*m*sizeel;
  void *y=(void *)malloc(nms);
  for (i=0;i<n;i++){
    for (j=0;j<m;j++){
      ji=sizeel*(n*j+i);
      memcpy((y+ji),x,sizeel);
      x+=sizeel;
    }
  }
  memcpy((x-nms),y,nms);
  if(y) free(y);
  return;
}
/**********************************************************************/
INT solve_pivot(INT dopivot, INT n, REAL *A, REAL *b, INT *p,REAL *piv)
{
  INT nm1,i1,k1,pin,kswp,kp,i,j,k;
  REAL r,t,absaij;
  REAL *x=piv;
  if(dopivot) {
    for (i=0;i<n;i++){
      p[i]=i;
      piv[i] = fabs(A[i*n+0]);
      for (j=1;j<n;j++){
      absaij=fabs(A[i*n+j]);
      if(absaij > piv[i])
	piv[i] = absaij;
      }
      piv[i]=1./piv[i]; //here we need error stop if too small
    }
    nm1 = n-1;
    for (k = 0;k<nm1;k++){
      r = fabs(A[p[k]*n+k])*piv[p[k]];
      kp = k;
      for (i=k;i<n;i++){
	t = fabs(A[p[i]*n+k])*piv[p[i]]; 
	if (t > r){r = t; kp = i;}
      }
      kswp = p[kp]; p[kp] = p[k]; p[k] = kswp;
      k1 = k+1; 
      for (i = k1;i<n;i++){
	pin=p[i]*n;
	A[pin+k] = A[pin+k]/A[p[k]*n+k];
	for(j = k1;j<n;j++){
	  A[pin+j] = A[pin+j]-A[pin+k]*A[p[k]*n+j];
	}
      }
    }
    /*end of decomposition; now solver part*/
  }
  x[0] = b[p[0]];
  for (i = 1;i<n;i++){
    x[i] = b[p[i]];
    for(j = 0;j<i;j++){
      x[i] = x[i]-A[p[i]*n+j]*x[j];
    }
  }
  nm1=n-1;
  x[nm1] = x[nm1]/A[p[nm1]*n+nm1];
  for (i = nm1;i>0;i--){
    i1=i-1;
    pin=p[i1]*n;
    for (j = i;j<n;j++){
      x[i1] = x[i1]-A[pin+j]*x[j];
    }
    x[i1] = x[i1]/A[pin+i1];
  }
  //  if(y) free(y);
  for(k=0;k<n;k++)b[k]=x[k];
  return 0;
}
/**************************************************************************/
/*
 * \fn void lufull(INT n, REAL *deta, REAL *A,INT *p,REAL *piv)
 *
 * \brief LU decomposition of an (nxn) matrix A. on output returns the
 *        value of the determinant of A as well. Uses scaled partial
 *        pivoting.
 *
 * \param n    Number of rows
 * \param deta determinant of A
 * \param A    the matrix as a one dim. array by rows
 * \param dopivot flag to indicate whether A is already decomposed in
 *                LU and we need to compute the determinant only;
 *
 */
void lufull(INT dopivot, INT n, REAL *deta, REAL *A,INT *p,REAL *piv)
{
  INT nm1,i1,k1,pin,kswp,kp,i,j,k;
  REAL det0,r,t,absaij;
  if(dopivot){
    for (i=0;i<n;i++){
      p[i]=i;
      piv[i] = fabs(A[i*n+0]);
      for (j=1;j<n;j++){
	absaij=fabs(A[i*n+j]);
	if(absaij > piv[i])
	  piv[i] = absaij;
      }
      piv[i]=1./piv[i]; //here we need error stop if too small
    }
    nm1 = n-1;
    for (k = 0;k<nm1;k++){
      r = fabs(A[p[k]*n+k])*piv[p[k]];
      kp = k;
      for (i=k;i<n;i++){
	t = fabs(A[p[i]*n+k])*piv[p[i]]; 
	if (t > r){r = t; kp = i;}
      }
      kswp = p[kp]; p[kp] = p[k]; p[k] = kswp;
      k1 = k+1; 
      for (i = k1;i<n;i++){
	pin=p[i]*n;
	A[pin+k] = A[pin+k]/A[p[k]*n+k];
	for(j = k1;j<n;j++){
	  A[pin+j] = A[pin+j]-A[pin+k]*A[p[k]*n+j];
	}
      }
    }
  }
  nm1=n-1;
  det0=A[p[nm1]*n+nm1];
  for (i = nm1;i>0;i--){
    i1=i-1;
    pin=p[i1]*n;
    det0 *= A[pin+i1];
  }
  *deta=det0;
  return;
}
/**************************************************************************/
/*
 * \fn void invfull(INT n, REAL *Ainv, REAL *A, void *wrk)
 *
 * \brief Inverts a general (nxn) matrix A
 *
 * \param n    Number of rows
 * \param m    Number of columns
 * \param A    the matrix as a one dim. array by rows
 * \param Ainv the output inverse. 
 * 
 * \param wrk working array of size at least
 *            n*sizeof(INT)+n*(n+1)*sizeof(REAL)
 *
 */
void invfull(REAL *Ainv,INT n, REAL *A, void *wrk)
{
  
  /*  cast pointers*/
  REAL *piv=(REAL *)wrk;  REAL *Awrk=piv+n;
  INT *p=(INT *)(wrk+n*(n+1)*sizeof(REAL));
  INT i,j,ji,ni;
  /* first transpose A beacuse we invert by rows;*/
  for (i=0;i<n;i++){
    for (j=0;j<n;j++){
      ji=(n*j+i);
      Awrk[ji]=*A;
      A++;
    }
  }
  //  print_full_mat(n,n,Awrk,"At");
  for(i=0;i<n;i++){
    ni=n*i;
    for(j=0;j<n;j++)
      Ainv[ni+j]=0e0;
    Ainv[ni+i]=1e0;
    // solve with rhs a row of the identity;
    solve_pivot((!i),n, Awrk, (Ainv+ni), p, piv);
  }
  return;
}
/**************************************************************************/
/*
 * \fn void abybfull(const INT m, const INT p, REAL *c, REAL *a, 
 *                   REAL *b, const INT n)
 *
 * \brief 
 *
 * \param 
 * \param 
 * \param 
 *
 */
void abybfull(const INT m, const INT p, REAL *c,	\
	      REAL *a, REAL *b, const INT n)
{
  /* matrices c = a*b+c; a is m by n, b is n by p; c is m by p */
  REAL cij;
  INT i,j,k,ij,ik,kj,in,ip;
  for(i=0;i<m;i++){
    in=i*n;
    ip=i*p;
    for(j=0;j<p;j++){
      cij=0.;
      for(k=0;k<n;k++){
	ik=in + k;
	kj=k*p + j;
	cij+=a[ik]*b[kj];
      }
      ij=ip+j;
      c[ij]+=cij;
    }
  }
  return;
}	      
/**************************************************************************/
/*
 * \fn void abyvfull(const INT m, REAL *y,REAL *a, REAL *x, const INT n)
 *
 * \brief 
 *
 * \param 
 * \param 
 * \param 
 *
 */
void abyvfull(const INT m, REAL *y,REAL *a, REAL *x, const INT n)
{
  /* matrices y = a*x+y; a is a matrix m by n, x is a vector, y is a
     vector (m) */
  REAL yi;
  INT i,j,in;
  for(i=0;i<m;i++){
    in=i*n; yi=0.;
    for(j=0;j<n;j++){
      yi+=a[in+j]*x[j];
    }
    y[i]+=yi;
  }
  return;
}	      
/**************************************************************************/
/*
 * \fn atbyvfull(const INT m, REAL *y,REAL *a, REAL *x, const INT n)
 *
 * \brief 
 *
 * \param 
 * \param 
 * \param 
 *
 */
void atbyvfull(const INT m, REAL *y,REAL *a, REAL *x, const INT n)
{
  /* matrices y = a^t*x+y; a is a matrix m by n, x is m by 1 vector, y
     is n by 1 */
  REAL yj;
  INT i,j,in;
  for(i=0;i<m;i++){
    in=i*n;
    for(j=0;j<n;j++){
      y[j]+=x[i]*a[in+j];
    }
  }
  return;
}	      
