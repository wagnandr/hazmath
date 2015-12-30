/*
 *  krylov.c
 *
 *  Created by James Adler and Xiaozhe Hu on 5/13/15.
 *  Copyright 2015__HAZMAT__. All rights reserved.
 *
 */

#include "hazmat.h"

#include "itsolver_util.inl"


/*! \brief Krylov subspace methods -- Preconditioned conjugate gradient
 *
 *  Abstract algorithm
 *
 *  PCG method to solve A*x=b is to generate {x_k} to approximate x
 *
 *  Step 0. Given A, b, x_0, M
 *
 *  Step 1. Compute residual r_0 = b-A*x_0 and convergence check;
 *
 *  Step 2. Initialization z_0 = M^{-1}*r_0, p_0=z_0;
 *
 *  Step 3. Main loop ...
 *
 *  FOR k = 0:MaxIt
 *      - get step size alpha = f(r_k,z_k,p_k);
 *      - update solution: x_{k+1} = x_k + alpha*p_k;
 *      - perform stagnation check;
 *      - update residual: r_{k+1} = r_k - alpha*(A*p_k);
 *      - perform residual check;
 *      - obtain p_{k+1} using {p_0, p_1, ... , p_k};
 *      - prepare for next iteration;
 *      - print the result of k-th iteration;
 *  END FOR
 *
 *  Convergence check: norm(r)/norm(b) < tol
 *
 *  Stagnation check:
 *      - IF norm(alpha*p_k)/norm(x_{k+1}) < tol_stag
 *          -# compute r=b-A*x_{k+1};
 *          -# convergence check;
 *          -# IF ( not converged & restart_number < Max_Stag_Check ) restart;
 *      - END IF
 *
 *  Residual check:
 *      - IF norm(r_{k+1})/norm(b) < tol
 *          -# compute the real residual r = b-A*x_{k+1};
 *          -# convergence check;
 *          -# IF ( not converged & restart_number < Max_Res_Check ) restart;
 *      - END IF
 *
 *  \note Refer to Y. Saad 2003
 *        Iterative methods for sparse linear systems (2nd Edition), SIAM
 *
 *  \note See spcg.c for a safer version
 */

/***********************************************************************************************/
INT dcsr_pcg (dCSRmat *A,
              dvector *b,
              dvector *u,
              precond *pc,
              const REAL tol,
              const INT MaxIt,
              const SHORT stop_type,
              const SHORT prtlvl)
{
  /**
   * \fn INT dcsr_pcg (dCSRmat *A, dvector *b, dvector *u, precond *pc,
   *                               const REAL tol, const INT MaxIt,
   *                               const SHORT stop_type, const SHORT prtlvl)
   *
   * \brief Preconditioned conjugate gradient method for solving Au=b
   *
   * \param A            Pointer to dCSRmat: the coefficient matrix
   * \param b            Pointer to dvector: the right hand side
   * \param u            Pointer to dvector: the unknowns
   * \param pc           Pointer to precond: the structure of precondition
   * \param tol          Tolerance for stopping
   * \param MaxIt        Maximal number of iterations
   * \param stop_type    Stopping criteria type
   * \param prtlvl       How much information to print out
   *
   * \return             Iteration number if converges; ERROR otherwise.
   *
   *
   */
    
  // Check if matrix counting from 1 or 0 for CSR arrays (James vs. Xiaozhe code)
  INT shift_flag = 0;
  if(A->IA[0]==1) {
    dcsr_shift(A, -1);  // shift A
    shift_flag = 1;
  }

  const SHORT  MaxStag = MAX_STAG, MaxRestartStep = MAX_RESTART;
  const INT    m = b->row;
  const REAL   maxdiff = tol*STAG_RATIO; // stagnation tolerance
  const REAL   sol_inf_tol = SMALLREAL; // infinity norm tolerance
    
  // local variables
  INT          iter = 0, stag = 1, more_step = 1, restart_step = 1;
  REAL         absres0 = BIGREAL, absres = BIGREAL;
  REAL         relres  = BIGREAL, normu  = BIGREAL, normr0 = BIGREAL;
  REAL         reldiff, factor, infnormu;
  REAL         alpha, beta, temp1, temp2;
    
  // allocate temp memory (need 4*m REAL numbers)
  REAL *work = (REAL *)calloc(4*m,sizeof(REAL));
  REAL *p = work, *z = work+m, *r = z+m, *t = r+m;
    
  //#if DEBUG_MODE > 0
  //    printf("### DEBUG: %s ...... [Start]\n", __FUNCTION__);
  //    printf("### DEBUG: maxit = %d, tol = %.4le\n", MaxIt, tol);
  //#endif
    
  // r = b-A*u
  array_cp(m,b->val,r);
  dcsr_aAxpy(-1.0,A,u->val,r);
    
  if ( pc != NULL )
    pc->fct(r,z,pc->data); /* Apply preconditioner */
  else
    array_cp(m,r,z); /* No preconditioner */
    
  // compute initial residuals
  switch ( stop_type ) {
  case STOP_REL_RES:
    absres0 = array_norm2(m,r);
    normr0  = MAX(SMALLREAL,absres0);
    relres  = absres0/normr0;
    break;
  case STOP_REL_PRECRES:
    absres0 = sqrt(array_dotprod(m,r,z));
    normr0  = MAX(SMALLREAL,absres0);
    relres  = absres0/normr0;
    break;
  case STOP_MOD_REL_RES:
    absres0 = array_norm2(m,r);
    normu   = MAX(SMALLREAL,array_norm2(m,u->val));
    relres  = absres0/normu;
    break;
  default:
    printf("### ERROR: Unrecognised stopping type for %s!\n", __FUNCTION__);
    goto FINISHED;
  }
    
  // if initial residual is small, no need to iterate!
  if ( relres < tol || absres0 < 1e-3*tol ) goto FINISHED;
    
  // output iteration information if needed
  print_itsolver_info(prtlvl,stop_type,iter,relres,absres0,0.0);
    
  array_cp(m,z,p);
  temp1 = array_dotprod(m,z,r);
    
  // main PCG loop
  while ( iter++ < MaxIt ) {
        
    // t=A*p
    dcsr_mxv(A,p,t);
        
    // alpha_k=(z_{k-1},r_{k-1})/(A*p_{k-1},p_{k-1})
    temp2 = array_dotprod(m,t,p);
    alpha = temp1/temp2;
        
    // u_k=u_{k-1} + alpha_k*p_{k-1}
    array_axpy(m,alpha,p,u->val);
        
    // r_k=r_{k-1} - alpha_k*A*p_{k-1}
    array_axpy(m,-alpha,t,r);
        
    // compute residuals
    switch ( stop_type ) {
    case STOP_REL_RES:
      absres = array_norm2(m,r);
      relres = absres/normr0;
      break;
    case STOP_REL_PRECRES:
      // z = B(r)
      if ( pc != NULL )
	pc->fct(r,z,pc->data); /* Apply preconditioner */
      else
	array_cp(m,r,z); /* No preconditioner */
      absres = sqrt(ABS(array_dotprod(m,z,r)));
      relres = absres/normr0;
      break;
    case STOP_MOD_REL_RES:
      absres = array_norm2(m,r);
      relres = absres/normu;
      break;
    }
        
    // compute reduction factor of residual ||r||
    factor = absres/absres0;
        
    // output iteration information if needed
    print_itsolver_info(prtlvl,stop_type,iter,relres,absres,factor);
        
    // Check I: if solution is close to zero, return ERROR_SOLVER_SOLSTAG
    infnormu = array_norminf(m, u->val);
    if ( infnormu <= sol_inf_tol ) {
      if ( prtlvl > PRINT_MIN ) ITS_ZEROSOL;
      iter = ERROR_SOLVER_SOLSTAG;
      break;
    }
        
    // Check II: if stagnated, try to restart
    normu   = dvec_norm2(u);
        
    // compute relative difference
    reldiff = ABS(alpha)*array_norm2(m,p)/normu;
    if ( (stag <= MaxStag) & (reldiff < maxdiff) ) {
            
      if ( prtlvl >= PRINT_MORE ) {
	ITS_DIFFRES(reldiff,relres);
	ITS_RESTART;
      }
            
      array_cp(m,b->val,r);
      dcsr_aAxpy(-1.0,A,u->val,r);
            
      // compute residuals
      switch ( stop_type ) {
      case STOP_REL_RES:
	absres = array_norm2(m,r);
	relres = absres/normr0;
	break;
      case STOP_REL_PRECRES:
	// z = B(r)
	if ( pc != NULL )
	  pc->fct(r,z,pc->data); /* Apply preconditioner */
	else
	  array_cp(m,r,z); /* No preconditioner */
	absres = sqrt(ABS(array_dotprod(m,z,r)));
	relres = absres/normr0;
	break;
      case STOP_MOD_REL_RES:
	absres = array_norm2(m,r);
	relres = absres/normu;
	break;
      }
            
      if ( prtlvl >= PRINT_MORE ) ITS_REALRES(relres);
            
      if ( relres < tol )
	break;
      else {
	if ( stag >= MaxStag ) {
	  if ( prtlvl > PRINT_MIN ) ITS_STAGGED;
	  iter = ERROR_SOLVER_STAG;
	  break;
	}
	array_set(m,p,0.0);
	++stag;
	++restart_step;
      }
    } // end of stagnation check!
        
    // Check III: prevent false convergence
    if ( relres < tol ) {
            
      REAL computed_relres = relres;
            
      // compute residual r = b - Ax again
      array_cp(m,b->val,r);
      dcsr_aAxpy(-1.0,A,u->val,r);
            
      // compute residuals
      switch ( stop_type ) {
      case STOP_REL_RES:
	absres = array_norm2(m,r);
	relres = absres/normr0;
	break;
      case STOP_REL_PRECRES:
	// z = B(r)
	if ( pc != NULL )
	  pc->fct(r,z,pc->data); /* Apply preconditioner */
	else
	  array_cp(m,r,z); /* No preconditioner */
	absres = sqrt(ABS(array_dotprod(m,z,r)));
	relres = absres/normr0;
	break;
      case STOP_MOD_REL_RES:
	absres = array_norm2(m,r);
	relres = absres/normu;
	break;
      }
            
      // check convergence
      if ( relres < tol ) break;
            
      if ( prtlvl >= PRINT_MORE ) {
	ITS_COMPRES(computed_relres); ITS_REALRES(relres);
      }
            
      if ( more_step >= MaxRestartStep ) {
	if ( prtlvl > PRINT_MIN ) ITS_ZEROTOL;
	iter = ERROR_SOLVER_TOLSMALL;
	break;
      }
            
      // prepare for restarting the method
      array_set(m,p,0.0);
      ++more_step;
      ++restart_step;
            
    } // end of safe-guard check!
        
    // save residual for next iteration
    absres0 = absres;
        
    // compute z_k = B(r_k)
    if ( stop_type != STOP_REL_PRECRES ) {
      if ( pc != NULL )
	pc->fct(r,z,pc->data); /* Apply preconditioner */
      else
	array_cp(m,r,z); /* No preconditioner, B=I */
    }
        
    // compute beta_k = (z_k, r_k)/(z_{k-1}, r_{k-1})
    temp2 = array_dotprod(m,z,r);
    beta  = temp2/temp1;
    temp1 = temp2;
        
    // compute p_k = z_k + beta_k*p_{k-1}
    array_axpby(m,1.0,z,beta,p);
        
  } // end of main PCG loop.
    
 FINISHED:  // finish the iterative method
  if ( prtlvl > PRINT_NONE ) ITS_FINAL(iter,MaxIt,relres);
    
  // clean up temp memory
  free(work);
    
#if DEBUG_MODE > 0
  printf("### DEBUG: %s ...... [Finish]\n", __FUNCTION__);
#endif

  // Fix A back to correct counting if needed
  if(shift_flag==1) {
    dcsr_shift(A, 1);  // shift A back
  }
    
  if ( iter > MaxIt )
    return ERROR_SOLVER_MAXIT;
  else
    return iter;
}

/***********************************************************************************************/
INT dcsr_pminres (dCSRmat *A,
                              dvector *b,
                              dvector *u,
                              precond *pc,
                              const REAL tol,
                              const INT MaxIt,
                              const SHORT stop_type,
                              const SHORT prtlvl)
{
    /**
     * \fn INT dcsr_pminres (dCSRmat *A, dvector *b, dvector *u, precond *pc,
     *                                   const REAL tol, const INT MaxIt,
     *                                   const SHORT stop_type, const SHORT prtlvl)
     *
     * \brief A preconditioned minimal residual (Minres) method for solving Au=b
     *
     * \param A            Pointer to dCSRmat: the coefficient matrix
     * \param b            Pointer to dvector: the right hand side
     * \param u            Pointer to dvector: the unknowns
     * \param pc           Pointer to precond: the structure of precondition
     * \param tol          Tolerance for stopping
     * \param MaxIt        Maximal number of iterations
     * \param stop_type    Stopping criteria type
     * \param prtlvl       How much information to print out
     *
     * \return             Iteration number if converges; ERROR otherwise.
     *
     * \author Xiaozhe Hu
     * \date   12/30/2015
     *
     * \note Rewritten based on the original version by Chensong Zhang 05/01/2012
     *
     */
    
    const SHORT  MaxStag = MAX_STAG, MaxRestartStep = MAX_RESTART;
    const INT    m = b->row;
    const REAL   maxdiff = tol*STAG_RATIO; // staganation tolerance
    const REAL   sol_inf_tol = SMALLREAL; // infinity norm tolerance
    
    // local variables
    INT          iter = 0, stag = 1, more_step = 1, restart_step = 1;
    REAL         absres0 = BIGREAL, absres = BIGREAL;
    REAL         normr0  = BIGREAL, relres  = BIGREAL;
    REAL         normu2, normuu, normp, infnormu, factor;
    REAL         alpha, alpha0, alpha1, temp2;
    
    // allocate temp memory (need 11*m REAL)
    REAL *work=(REAL *)calloc(11*m,sizeof(REAL));
    REAL *p0=work, *p1=work+m, *p2=p1+m, *z0=p2+m, *z1=z0+m;
    REAL *t0=z1+m, *t1=t0+m, *t=t1+m, *tp=t+m, *tz=tp+m, *r=tz+m;
    
    // p0 = 0
    array_set(m,p0,0.0);
    
    // r = b-A*u
    array_cp(m,b->val,r);
    dcsr_aAxpy(-1.0,A,u->val,r);
    
    // p1 = B(r)
    if ( pc != NULL )
        pc->fct(r,p1,pc->data); /* Apply preconditioner */
    else
        array_cp(m,r,p1); /* No preconditioner */
    
    // compute initial residuals
    switch ( stop_type ) {
        case STOP_REL_RES:
            absres0 = array_norm2(m,r);
            normr0  = MAX(SMALLREAL,absres0);
            relres  = absres0/normr0;
            break;
        case STOP_REL_PRECRES:
            absres0 = sqrt(array_dotprod(m,r,p1));
            normr0  = MAX(SMALLREAL,absres0);
            relres  = absres0/normr0;
            break;
        case STOP_MOD_REL_RES:
            absres0 = array_norm2(m,r);
            normu2  = MAX(SMALLREAL,array_norm2(m,u->val));
            relres  = absres0/normu2;
            break;
        default:
            printf("### ERROR: Unrecognized stopping type for %s!\n", __FUNCTION__);
            goto FINISHED;
    }
    
    // if initial residual is small, no need to iterate!
    if ( relres < tol || absres0 < 1e-3*tol ) goto FINISHED;
    
    // output iteration information if needed
    print_itsolver_info(prtlvl,stop_type,iter,relres,absres0,0.0);
    
    // tp = A*p1
    dcsr_mxv(A,p1,tp);
    
    // tz = B(tp)
    if ( pc != NULL )
        pc->fct(tp,tz,pc->data); /* Apply preconditioner */
    else
        array_cp(m,tp,tz); /* No preconditioner */
    
    // p1 = p1/normp
    normp = ABS(array_dotprod(m,tz,tp));
    normp = sqrt(normp);
    array_cp(m,p1,t);
    array_set(m,p1,0.0);
    array_axpy(m,1/normp,t,p1);
    
    // t0 = A*p0 = 0
    array_set(m,t0,0.0);
    array_cp(m,t0,z0);
    array_cp(m,t0,t1);
    array_cp(m,t0,z1);
    
    // t1 = tp/normp, z1 = tz/normp
    array_axpy(m,1.0/normp,tp,t1);
    array_axpy(m,1.0/normp,tz,z1);
    
    // main MinRes loop
    while ( iter++ < MaxIt ) {
        
        // alpha = <r,z1>
        alpha = array_dotprod(m,r,z1);
        
        // u = u+alpha*p1
        array_axpy(m,alpha,p1,u->val);
        
        // r = r-alpha*Ap1
        array_axpy(m,-alpha,t1,r);
        
        // compute t = A*z1 alpha1 = <z1,t>
        dcsr_mxv(A,z1,t);
        alpha1 = array_dotprod(m,z1,t);
        
        // compute t = A*z0 alpha0 = <z1,t>
        dcsr_mxv(A,z0,t);
        alpha0 = array_dotprod(m,z1,t);
        
        // p2 = z1-alpha1*p1-alpha0*p0
        array_cp(m,z1,p2);
        array_axpy(m,-alpha1,p1,p2);
        array_axpy(m,-alpha0,p0,p2);
        
        // tp = A*p2
        dcsr_mxv(A,p2,tp);
        
        // tz = B(tp)
        if ( pc != NULL )
            pc->fct(tp,tz,pc->data); /* Apply preconditioner */
        else
            array_cp(m,tp,tz); /* No preconditioner */
        
        // p2 = p2/normp
        normp = ABS(array_dotprod(m,tz,tp));
        normp = sqrt(normp);
        array_cp(m,p2,t);
        array_set(m,p2,0.0);
        array_axpy(m,1/normp,t,p2);
        
        // prepare for the next iteration
        array_cp(m,p1,p0);
        array_cp(m,p2,p1);
        array_cp(m,t1,t0);
        array_cp(m,z1,z0);
        
        // t1=tp/normp,z1=tz/normp
        array_set(m,t1,0.0);
        array_cp(m,t1,z1);
        array_axpy(m,1/normp,tp,t1);
        array_axpy(m,1/normp,tz,z1);
        
        normu2 = array_norm2(m,u->val);
        
        // compute residuals
        switch ( stop_type ) {
            case STOP_REL_RES:
                temp2  = array_dotprod(m,r,r);
                absres = sqrt(temp2);
                relres = absres/normr0;
                break;
            case STOP_REL_PRECRES:
                if (pc == NULL)
                    array_cp(m,r,t);
                else
                    pc->fct(r,t,pc->data);
                temp2  = ABS(array_dotprod(m,r,t));
                absres = sqrt(temp2);
                relres = absres/normr0;
                break;
            case STOP_MOD_REL_RES:
                temp2  = array_dotprod(m,r,r);
                absres = sqrt(temp2);
                relres = absres/normu2;
                break;
        }
        
        // compute reducation factor of residual ||r||
        factor = absres/absres0;
        
        // output iteration information if needed
        print_itsolver_info(prtlvl,stop_type,iter,relres,absres,factor);
        
        // Check I: if soultion is close to zero, return ERROR_SOLVER_SOLSTAG
        infnormu = array_norminf(m, u->val);
        if (infnormu <= sol_inf_tol) {
            if ( prtlvl > PRINT_MIN ) ITS_ZEROSOL;
            iter = ERROR_SOLVER_SOLSTAG;
            break;
        }
        
        // Check II: if staggenated, try to restart
        normuu = array_norm2(m,p1);
        normuu = ABS(alpha)*(normuu/normu2);
        
        if ( normuu < maxdiff ) {
            
            if ( stag < MaxStag ) {
                if ( prtlvl >= PRINT_MORE ) {
                    ITS_DIFFRES(normuu,relres);
                    ITS_RESTART;
                }
            }
            
            array_cp(m,b->val,r);
            dcsr_aAxpy(-1.0,A,u->val,r);
            
            // compute residuals
            switch (stop_type) {
                case STOP_REL_RES:
                    temp2  = array_dotprod(m,r,r);
                    absres = sqrt(temp2);
                    relres = absres/normr0;
                    break;
                case STOP_REL_PRECRES:
                    if (pc == NULL)
                        array_cp(m,r,t);
                    else
                        pc->fct(r,t,pc->data);
                    temp2  = ABS(array_dotprod(m,r,t));
                    absres = sqrt(temp2);
                    relres = absres/normr0;
                    break;
                case STOP_MOD_REL_RES:
                    temp2  = array_dotprod(m,r,r);
                    absres = sqrt(temp2);
                    relres = absres/normu2;
                    break;
            }
            
            if ( prtlvl >= PRINT_MORE ) ITS_REALRES(relres);
            
            if ( relres < tol )
                break;
            else {
                if ( stag >= MaxStag ) {
                    if ( prtlvl > PRINT_MIN ) ITS_STAGGED;
                    iter = ERROR_SOLVER_STAG;
                    break;
                }
                array_set(m,p0,0.0);
                ++stag;
                ++restart_step;
                
                // p1 = B(r)
                if ( pc != NULL )
                    pc->fct(r,p1,pc->data); /* Apply preconditioner */
                else
                    array_cp(m,r,p1); /* No preconditioner */
                
                // tp = A*p1
                dcsr_mxv(A,p1,tp);
                
                // tz = B(tp)
                if ( pc != NULL )
                    pc->fct(tp,tz,pc->data); /* Apply rreconditioner */
                else
                    array_cp(m,tp,tz); /* No preconditioner */
                
                // p1 = p1/normp
                normp = array_dotprod(m,tz,tp);
                normp = sqrt(normp);
                array_cp(m,p1,t);
                
                // t0 = A*p0=0
                array_set(m,t0,0.0);
                array_cp(m,t0,z0);
                array_cp(m,t0,t1);
                array_cp(m,t0,z1);
                array_cp(m,t0,p1);
                
                array_axpy(m,1/normp,t,p1);
                
                // t1 = tp/normp, z1 = tz/normp
                array_axpy(m,1/normp,tp,t1);
                array_axpy(m,1/normp,tz,z1);
            }
        }
        
        // Check III: prevent false convergence
        if ( relres < tol ) {
            
            if ( prtlvl >= PRINT_MORE ) ITS_COMPRES(relres);
            
            // compute residual r = b - Ax again
            array_cp(m,b->val,r);
            dcsr_aAxpy(-1.0,A,u->val,r);
            
            // compute residuals
            switch (stop_type) {
                case STOP_REL_RES:
                    temp2  = array_dotprod(m,r,r);
                    absres = sqrt(temp2);
                    relres = absres/normr0;
                    break;
                case STOP_REL_PRECRES:
                    if (pc == NULL)
                        array_cp(m,r,t);
                    else
                        pc->fct(r,t,pc->data);
                    temp2  = ABS(array_dotprod(m,r,t));
                    absres = sqrt(temp2);
                    relres = absres/normr0;
                    break;
                case STOP_MOD_REL_RES:
                    temp2  = array_dotprod(m,r,r);
                    absres = sqrt(temp2);
                    relres = absres/normu2;
                    break;
            }
            
            if ( prtlvl >= PRINT_MORE ) ITS_REALRES(relres);
            
            // check convergence
            if ( relres < tol ) break;
            
            if ( more_step >= MaxRestartStep ) {
                if ( prtlvl > PRINT_MIN ) ITS_ZEROTOL;
                iter = ERROR_SOLVER_TOLSMALL;
                break;
            }
            
            // prepare for restarting the method
            array_set(m,p0,0.0);
            ++more_step;
            ++restart_step;
            
            // p1 = B(r)
            if ( pc != NULL )
                pc->fct(r,p1,pc->data); /* Apply preconditioner */
            else
                array_cp(m,r,p1); /* No preconditioner */
            
            // tp = A*p1
            dcsr_mxv(A,p1,tp);
            
            // tz = B(tp)
            if ( pc != NULL )
                pc->fct(tp,tz,pc->data); /* Apply rreconditioner */
            else
                array_cp(m,tp,tz); /* No preconditioner */
            
            // p1 = p1/normp
            normp = array_dotprod(m,tz,tp);
            normp = sqrt(normp);
            array_cp(m,p1,t);
            
            // t0 = A*p0 = 0
            array_set(m,t0,0.0);
            array_cp(m,t0,z0);
            array_cp(m,t0,t1);
            array_cp(m,t0,z1);
            array_cp(m,t0,p1);
            
            array_axpy(m,1/normp,t,p1);
            
            // t1=tp/normp,z1=tz/normp
            array_axpy(m,1/normp,tp,t1);
            array_axpy(m,1/normp,tz,z1);
            
        } // end of convergence check
        
        // update relative residual here
        absres0 = absres;
        
    } // end of the main loop
    
FINISHED:  // finish the iterative method
    if ( prtlvl > PRINT_NONE ) ITS_FINAL(iter,MaxIt,relres);
    
    // clean up temp memory
    free(work);
    
#if DEBUG_MODE > 0
    printf("### DEBUG: %s ...... [Finish]\n", __FUNCTION__);
#endif
    
    if ( iter > MaxIt )
        return ERROR_SOLVER_MAXIT;
    else
        return iter;
}

/***********************************************************************************************/
INT dcsr_pvgmres (dCSRmat *A,
                  dvector *b,
                  dvector *x,
                  precond *pc,
                  const REAL tol,
                  const INT MaxIt,
                  const SHORT restart,
                  const SHORT stop_type,
                  const SHORT prtlvl)
{
  /*!
   * \fn INT dcsr_pvgmres (dCSRmat *A, dvector *b, dvector *x, precond *pc,
   *                                   const REAL tol, const INT MaxIt, const SHORT restart,
   *                                   const SHORT stop_type, const SHORT prtlvl)
   *
   * \brief Right preconditioned GMRES method in which the restart parameter
   *        can be adaptively modified during the iteration.
   *
   * \param A            Pointer to dCSRmat: the coefficient matrix
   * \param b            Pointer to dvector: the right hand side
   * \param x            Pointer to dvector: the unknowns
   * \param pc           Pointer to precond: the structure of precondition
   * \param tol          Tolerance for stopping
   * \param MaxIt        Maximal number of iterations
   * \param restart      Restarting steps
   * \param stop_type    Stopping criteria type
   * \param prtlvl       How much information to print out
   *
   * \return             Iteration number if converges; ERROR otherwise.
   *
   * \note Refer to A.H. Baker, E.R. Jessup, and Tz.V. Kolev
   *       A Simple Strategy for Varying the Restart Parameter in GMRES(m)
   *       Journal of Computational and Applied Mathematics, 230 (2009)
   *       pp. 751-761. UCRL-JRNL-235266.
   *
   */
    
  // Check if matrix counting from 1 or 0 for CSR arrays (James vs. Xiaozhe code)
  INT shift_flag = 0;
  if(A->IA[0]==1) {
    dcsr_shift(A, -1);  // shift A
    shift_flag = 1;
  }

  const INT   n          = b->row;
  const INT   MIN_ITER   = 0;
  const REAL  epsmac     = SMALLREAL;
    
  //--------------------------------------------//
  //   Newly added parameters to monitor when   //
  //   to change the restart parameter          //
  //--------------------------------------------//
  const REAL cr_max      = 0.99;    // = cos(8^o)  (experimental)
  const REAL cr_min      = 0.174;   // = cos(80^o) (experimental)
    
  // local variables
  INT    iter            = 0;
  INT    i, j, k;
    
  REAL   r_norm, r_normb, gamma, t;
  REAL   absres0 = BIGREAL, absres = BIGREAL;
  REAL   relres  = BIGREAL, normu  = BIGREAL;
    
  REAL   cr           = 1.0;     // convergence rate
  REAL   r_norm_old   = 0.0;     // save the residual norm of the previous restart cycle
  INT    d            = 3;       // reduction for the restart parameter
  INT    restart_max  = restart; // upper bound for restart in each restart cycle
  INT    restart_min  = 3;       // lower bound for restart in each restart cycle
    
  unsigned INT  Restart  = restart; // the real restart in some fixed restarted cycle
  unsigned INT  Restart1 = Restart + 1;
  unsigned LONG worksize = (Restart+4)*(Restart+n)+1-n;
    
  // allocate temp memory (need about (restart+4)*n REAL numbers)
  REAL  *c = NULL, *s = NULL, *rs = NULL;
  REAL  *norms = NULL, *r = NULL, *w = NULL;
  REAL  *work = NULL;
  REAL  **p = NULL, **hh = NULL;
    
  //#if DEBUG_MODE > 0
  //    printf("### DEBUG: %s ...... [Start]\n", __FUNCTION__);
  //    printf("### DEBUG: maxit = %d, tol = %.4le\n", MaxIt, tol);
  //#endif
    
  /* allocate memory and setup temp work space */
  work  = (REAL *)calloc(worksize, sizeof(REAL));
    
  /* check whether memory is enough for GMRES */
  while ( (work == NULL) && (Restart > 5) ) {
    Restart = Restart - 5;
    worksize = (Restart+4)*(Restart+n)+1-n;
    work = (REAL *)calloc(worksize, sizeof(REAL));
    Restart1 = Restart + 1;
  }
    
  if ( work == NULL ) {
    printf("### ERROR: No enough memory for vGMRES %s : %s : %d!\n",
	   __FILE__, __FUNCTION__, __LINE__ );
    exit(ERROR_ALLOC_MEM);
  }
    
  if ( prtlvl > PRINT_MIN && Restart < restart ) {
    printf("### WARNING: vGMRES restart number set to %d!\n", Restart);
  }
    
  p     = (REAL **)calloc(Restart1, sizeof(REAL *));
  hh    = (REAL **)calloc(Restart1, sizeof(REAL *));
  norms = (REAL *) calloc(MaxIt+1, sizeof(REAL));
    
  r = work; w = r + n; rs = w + n; c = rs + Restart1; s = c + Restart;
    
  for ( i = 0; i < Restart1; i++ ) p[i] = s + Restart + i*n;
    
  for ( i = 0; i < Restart1; i++ ) hh[i] = p[Restart] + n + i*Restart;
    
  // r = b-A*x
  array_cp(n, b->val, p[0]);
  dcsr_aAxpy(-1.0, A, x->val, p[0]);
    
  r_norm = array_norm2(n, p[0]);
    
  // compute initial residuals
  switch (stop_type) {
  case STOP_REL_RES:
    absres0 = MAX(SMALLREAL,r_norm);
    relres  = r_norm/absres0;
    break;
  case STOP_REL_PRECRES:
    if ( pc == NULL )
      array_cp(n, p[0], r);
    else
      pc->fct(p[0], r, pc->data);
    r_normb = sqrt(array_dotprod(n,p[0],r));
    absres0 = MAX(SMALLREAL,r_normb);
    relres  = r_normb/absres0;
    break;
  case STOP_MOD_REL_RES:
    normu   = MAX(SMALLREAL,array_norm2(n,x->val));
    absres0 = r_norm;
    relres  = absres0/normu;
    break;
  default:
    printf("### ERROR: Unrecognised stopping type for %s!\n", __FUNCTION__);
    goto FINISHED;
  }
    
  // if initial residual is small, no need to iterate!
  if ( relres < tol || absres0 < 1e-3*tol ) goto FINISHED;
    
  // output iteration information if needed
  print_itsolver_info(prtlvl,stop_type,0,relres,absres0,0);
    
  // store initial residual
  norms[0] = relres;
    
  /* outer iteration cycle */
  while ( iter < MaxIt ) {
        
    rs[0] = r_norm_old = r_norm;
        
    t = 1.0 / r_norm;
        
    array_ax(n, t, p[0]);
        
    //-----------------------------------//
    //   adjust the restart parameter    //
    //-----------------------------------//
    if ( cr > cr_max || iter == 0 ) {
      Restart = restart_max;
    }
    else if ( cr < cr_min ) {
      // Restart = Restart;
    }
    else {
      if ( Restart - d > restart_min ) {
	Restart -= d;
      }
      else {
	Restart = restart_max;
      }
    }
        
    /* RESTART CYCLE (right-preconditioning) */
    i = 0;
    while ( i < Restart && iter < MaxIt ) {
            
      i++;  iter++;
            
      /* apply the preconditioner */
      if (pc == NULL)
	array_cp(n, p[i-1], r);
      else
	pc->fct(p[i-1], r, pc->data);
            
      dcsr_mxv(A, r, p[i]);
            
      /* modified Gram_Schmidt */
      for (j = 0; j < i; j ++) {
	hh[j][i-1] = array_dotprod(n, p[j], p[i]);
	array_axpy(n, -hh[j][i-1], p[j], p[i]);
      }
      t = array_norm2(n, p[i]);
      hh[i][i-1] = t;
      if (t != 0.0) {
	t = 1.0/t;
	array_ax(n, t, p[i]);
      }
            
      for (j = 1; j < i; ++j) {
	t = hh[j-1][i-1];
	hh[j-1][i-1] = s[j-1]*hh[j][i-1] + c[j-1]*t;
	hh[j][i-1] = -s[j-1]*t + c[j-1]*hh[j][i-1];
      }
      t= hh[i][i-1]*hh[i][i-1];
      t+= hh[i-1][i-1]*hh[i-1][i-1];
            
      gamma = sqrt(t);
      if (gamma == 0.0) gamma = epsmac;
      c[i-1]  = hh[i-1][i-1] / gamma;
      s[i-1]  = hh[i][i-1] / gamma;
      rs[i]   = -s[i-1]*rs[i-1];
      rs[i-1] = c[i-1]*rs[i-1];
      hh[i-1][i-1] = s[i-1]*hh[i][i-1] + c[i-1]*hh[i-1][i-1];
            
      absres = r_norm = fabs(rs[i]);
            
      relres = absres/absres0;
            
      norms[iter] = relres;
            
      // output iteration information if needed
      print_itsolver_info(prtlvl, stop_type, iter, relres, absres,
			  norms[iter]/norms[iter-1]);
            
      // should we exit the restart cycle
      if ( relres < tol && iter >= MIN_ITER ) break;
            
    } /* end of restart cycle */
        
    /* now compute solution, first solve upper triangular system */
    rs[i-1] = rs[i-1] / hh[i-1][i-1];
    for (k = i-2; k >= 0; k --) {
      t = 0.0;
      for (j = k+1; j < i; j ++)  t -= hh[k][j]*rs[j];
            
      t += rs[k];
      rs[k] = t / hh[k][k];
    }
        
    array_cp(n, p[i-1], w);
        
    array_ax(n, rs[i-1], w);
        
    for ( j = i-2; j >= 0; j-- ) array_axpy(n, rs[j], p[j], w);
        
    /* apply the preconditioner */
    if ( pc == NULL )
      array_cp(n, w, r);
    else
      pc->fct(w, r, pc->data);
        
    array_axpy(n, 1.0, r, x->val);
        
    // Check: prevent false convergence
    if ( relres < tol && iter >= MIN_ITER ) {
            
      REAL computed_relres = relres;
            
      // compute current residual
      array_cp(n, b->val, r);
      dcsr_aAxpy(-1.0, A, x->val, r);
            
      r_norm = array_norm2(n, r);
            
      switch ( stop_type ) {
      case STOP_REL_RES:
	absres = r_norm;
	relres = absres/absres0;
	break;
      case STOP_REL_PRECRES:
	if ( pc == NULL )
	  array_cp(n, r, w);
	else
	  pc->fct(r, w, pc->data);
	absres = sqrt(array_dotprod(n,w,r));
	relres = absres/absres0;
	break;
      case STOP_MOD_REL_RES:
	absres = r_norm;
	normu  = MAX(SMALLREAL,array_norm2(n,x->val));
	relres = absres/normu;
	break;
      }
            
      norms[iter] = relres;
            
      if ( relres < tol ) {
	break;
      }
      else {
	// Need to restart
	array_cp(n, r, p[0]); i = 0;
      }
            
      if ( prtlvl >= PRINT_MORE ) {
	ITS_COMPRES(computed_relres); ITS_REALRES(relres);
      }
            
    } /* end of convergence check */
        
    /* compute residual vector and continue loop */
    for ( j = i; j > 0; j-- ) {
      rs[j-1] = -s[j-1]*rs[j];
      rs[j]   = c[j-1]*rs[j];
    }
        
    if ( i ) array_axpy(n, rs[i]-1.0, p[i], p[i]);
        
    for ( j = i-1 ; j > 0; j-- ) array_axpy(n, rs[j], p[j], p[i]);
        
    if ( i ) {
      array_axpy(n, rs[0]-1.0, p[0], p[0]);
      array_axpy(n, 1.0, p[i], p[0]);
    }
        
    //-----------------------------------//
    //   compute the convergence rate    //
    //-----------------------------------//
    cr = r_norm / r_norm_old;
        
  } /* end of iteration while loop */
    
 FINISHED:
  if ( prtlvl > PRINT_NONE ) ITS_FINAL(iter,MaxIt,relres);
    
  /*-------------------------------------------
   * Free some stuff
   *------------------------------------------*/
  free(work);
  free(p);
  free(hh);
  free(norms);
    
  //#if DEBUG_MODE > 0
  //    printf("### DEBUG: %s ...... [Finish]\n", __FUNCTION__);
  //#endif

  // Fix A back to correct counting if needed
  if(shift_flag==1) {
    dcsr_shift(A, 1);  // shift A back
  }
    
  if (iter>=MaxIt)
    return ERROR_SOLVER_MAXIT;
  else
    return iter;
}

/***********************************************************************************************/
INT dcsr_pvfgmres (dCSRmat *A,
                   dvector *b,
                   dvector *x,
                   precond *pc,
                   const REAL tol,
                   const INT MaxIt,
                   const SHORT restart,
                   const SHORT stop_type,
                   const SHORT prtlvl)
{
    /*!
     * \fn INT dcsr_pvfgmres (dCSRmat *A, dvector *b, dvector *x, precond *pc,
     *                                    const REAL tol, const INT MaxIt, const SHORT restart,
     *                                    const SHORT stop_type, const SHORT prtlvl)
     *
     * \brief Solve "Ax=b" using PFGMRES(right preconditioned) iterative method in which
     *        the restart parameter can be adaptively modified during the iteration and
     *        flexible preconditioner can be used.
     *
     * \param A            Pointer to dCSRmat: the coefficient matrix
     * \param b            Pointer to dvector: the right hand side
     * \param x            Pointer to dvector: the unknowns
     * \param pc           Pointer to precond: the structure of precondition
     * \param tol          Tolerance for stopping
     * \param MaxIt        Maximal number of iterations
     * \param restart      Restarting steps
     * \param stop_type    Stopping criteria type -- DOES not support this parameter
     * \param prtlvl       How much information to print out
     *
     * \return             Iteration number if converges; ERROR otherwise.
     *
     * \author Xiaozhe Hu
     * \date   01/04/2012
     *
     */
    
    const INT n                 = b->row;
    const INT min_iter          = 0;
    
    //--------------------------------------------//
    //   Newly added parameters to monitor when   //
    //   to change the restart parameter          //
    //--------------------------------------------//
    const REAL cr_max           = 0.99;    // = cos(8^o)  (experimental)
    const REAL cr_min           = 0.174;   // = cos(80^o) (experimental)
    
    // local variables
    INT    iter                 = 0;
    INT    i,j,k;
    
    REAL   epsmac               = SMALLREAL;
    REAL   r_norm, b_norm, den_norm;
    REAL   epsilon, gamma, t;
    REAL   relres, normu, r_normb;
    
    REAL   *c = NULL, *s = NULL, *rs = NULL, *norms = NULL, *r = NULL;
    REAL   **p = NULL, **hh = NULL, **z=NULL;
    
    REAL   cr          = 1.0;     // convergence rate
    REAL   r_norm_old  = 0.0;     // save the residual norm of the previous restart cycle
    INT    d           = 3;       // reduction for the restart parameter
    INT    restart_max = restart; // upper bound for restart in each restart cycle
    INT    restart_min = 3;       // lower bound for restart in each restart cycle
    
    unsigned INT  Restart  = restart; // the real restart in some fixed restarted cycle
    unsigned INT  Restart1 = Restart + 1;
    unsigned LONG worksize = (Restart+4)*(Restart+n)+1-n+Restart*n;
    
    /* allocate memory and setup temp work space */
    REAL *work  = (REAL *) calloc(worksize, sizeof(REAL));
    
    /* check whether memory is enough for GMRES */
    while ( (work == NULL) && (Restart > 5) ) {
        Restart = Restart - 5;
        worksize = (Restart+4)*(Restart+n)+1-n+Restart*n;
        work = (REAL *) calloc(worksize, sizeof(REAL));
        Restart1 = Restart + 1;
    }
    
    if ( work == NULL ) {
        printf("### ERROR: No enough memory for vFGMRES %s : %s : %d!\n",
               __FILE__, __FUNCTION__, __LINE__ );
        exit(ERROR_ALLOC_MEM);
    }
    
    if ( prtlvl > PRINT_MIN && Restart < restart ) {
        printf("### WARNING: vFGMRES restart number set to %d!\n", Restart);
    }
    
    p  = (REAL **)calloc(Restart1, sizeof(REAL *));
    hh = (REAL **)calloc(Restart1, sizeof(REAL *));
    z  = (REAL **)calloc(Restart1, sizeof(REAL *));
    norms = (REAL *)calloc(MaxIt+1, sizeof(REAL));
    
    r = work; rs = r + n; c = rs + Restart1; s = c + Restart;
    for ( i = 0; i < Restart1; i++ ) p[i] = s + Restart + i*n;
    for ( i = 0; i < Restart1; i++ ) hh[i] = p[Restart] + n + i*Restart;
    for ( i = 0; i < Restart1; i++ ) z[i] = hh[Restart] + Restart + i*n;
    
    /* initialization */
    array_cp(n, b->val, p[0]);
    dcsr_aAxpy(-1.0, A, x->val, p[0]);
    
    b_norm = array_norm2(n, b->val);
    r_norm = array_norm2(n, p[0]);
    norms[0] = r_norm;
    
    if ( prtlvl >= PRINT_SOME) {
        ITS_PUTNORM("right-hand side", b_norm);
        ITS_PUTNORM("residual", r_norm);
    }
    
    if ( b_norm > 0.0 ) den_norm = b_norm;
    else                den_norm = r_norm;
    
    epsilon = tol*den_norm;
    
    // if initial residual is small, no need to iterate!
    if ( r_norm < epsilon || r_norm < 1e-3*tol ) goto FINISHED;
    
    if ( b_norm > 0.0 ) {
        print_itsolver_info(prtlvl,stop_type,iter,norms[iter]/b_norm,norms[iter],0);
    }
    else {
        print_itsolver_info(prtlvl,stop_type,iter,norms[iter],norms[iter],0);
    }
    
    /* outer iteration cycle */
    while ( iter < MaxIt ) {
        
        rs[0] = r_norm;
        r_norm_old = r_norm;
        if ( r_norm == 0.0 ) {
            free(work);
            free(p);
            free(hh);
            free(norms);
            free(z);
            return iter;
        }
        
        //-----------------------------------//
        //   adjust the restart parameter    //
        //-----------------------------------//
        
        if ( cr > cr_max || iter == 0 ) {
            Restart = restart_max;
        }
        else if ( cr < cr_min ) {
            // Restart = Restart;
        }
        else {
            if ( Restart - d > restart_min ) {
                Restart -= d;
            }
            else {
                Restart = restart_max;
            }
        }
        
        // Always entry the cycle at the first iteration
        // For at least one iteration step
        t = 1.0 / r_norm;
        array_ax(n, t, p[0]);
        i = 0;
        
        // RESTART CYCLE (right-preconditioning)
        while ( i < Restart && iter < MaxIt ) {
            
            i ++;  iter ++;
            
            /* apply the preconditioner */
            if ( pc == NULL )
                array_cp(n, p[i-1], z[i-1]);
            else
                pc->fct(p[i-1], z[i-1], pc->data);
            
            dcsr_mxv(A, z[i-1], p[i]);
            
            /* modified Gram_Schmidt */
            for ( j = 0; j < i; j++ ) {
                hh[j][i-1] = array_dotprod(n, p[j], p[i]);
                array_axpy(n, -hh[j][i-1], p[j], p[i]);
            }
            t = array_norm2(n, p[i]);
            hh[i][i-1] = t;
            if ( t != 0.0 ) {
                t = 1.0 / t;
                array_ax(n, t, p[i]);
            }
            
            for ( j = 1; j < i; ++j ) {
                t = hh[j-1][i-1];
                hh[j-1][i-1] = s[j-1]*hh[j][i-1] + c[j-1]*t;
                hh[j][i-1] = -s[j-1]*t + c[j-1]*hh[j][i-1];
            }
            t= hh[i][i-1]*hh[i][i-1];
            t+= hh[i-1][i-1]*hh[i-1][i-1];
            gamma = sqrt(t);
            if (gamma == 0.0) gamma = epsmac;
            c[i-1]  = hh[i-1][i-1] / gamma;
            s[i-1]  = hh[i][i-1] / gamma;
            rs[i]   = -s[i-1]*rs[i-1];
            rs[i-1] = c[i-1]*rs[i-1];
            hh[i-1][i-1] = s[i-1]*hh[i][i-1] + c[i-1]*hh[i-1][i-1];
            
            r_norm = fabs(rs[i]);
            norms[iter] = r_norm;
            
            if ( b_norm > 0 ) {
                print_itsolver_info(prtlvl,stop_type,iter,norms[iter]/b_norm,
                             norms[iter],norms[iter]/norms[iter-1]);
            }
            else {
                print_itsolver_info(prtlvl,stop_type,iter,norms[iter],norms[iter],
                             norms[iter]/norms[iter-1]);
            }
            
            /* should we exit the restart cycle? */
            if (r_norm <= epsilon && iter >= min_iter) break;
            
        } /* end of restart cycle */
        
        /* now compute solution, first solve upper triangular system */
        
        rs[i-1] = rs[i-1] / hh[i-1][i-1];
        for ( k = i-2; k >= 0; k-- ) {
            t = 0.0;
            for (j = k+1; j < i; j ++)  t -= hh[k][j]*rs[j];
            
            t += rs[k];
            rs[k] = t / hh[k][k];
        }
        
        array_cp(n, z[i-1], r);
        array_ax(n, rs[i-1], r);
        
        for ( j = i-2; j >= 0; j-- ) array_axpy(n, rs[j], z[j], r);
        
        array_axpy(n, 1.0, r, x->val);
        
        if ( r_norm <= epsilon && iter >= min_iter ) {
            array_cp(n, b->val, r);
            dcsr_aAxpy(-1.0, A, x->val, r);
            r_norm = array_norm2(n, r);
            
            switch (stop_type) {
                case STOP_REL_RES:
                    relres  = r_norm/den_norm;
                    break;
                case STOP_REL_PRECRES:
                    if ( pc == NULL )
                        array_cp(n, r, p[0]);
                    else
                        pc->fct(r, p[0], pc->data);
                    r_normb = sqrt(array_dotprod(n,p[0],r));
                    relres  = r_normb/den_norm;
                    break;
                case STOP_MOD_REL_RES:
                    normu   = MAX(SMALLREAL,array_norm2(n,x->val));
                    relres  = r_norm/normu;
                    break;
                default:
                    printf("### ERROR: Unrecognised stopping type for %s!\n", __FUNCTION__);
                    goto FINISHED;
            }
            
            if ( relres <= tol ) {
                break;
            }
            else {
                if ( prtlvl >= PRINT_SOME ) ITS_FACONV;
                array_cp(n, r, p[0]); i = 0;
            }
            
        } /* end of convergence check */
        
        /* compute residual vector and continue loop */
        for ( j = i; j > 0; j-- ) {
            rs[j-1] = -s[j-1]*rs[j];
            rs[j] = c[j-1]*rs[j];
        }
        
        if (i) array_axpy(n, rs[i]-1.0, p[i], p[i]);
        
        for ( j = i-1; j > 0; j-- ) array_axpy(n, rs[j], p[j], p[i]);
        
        if (i) {
            array_axpy(n, rs[0]-1.0, p[0], p[0]);
            array_axpy(n, 1.0, p[i], p[0]);
        }
        
        //-----------------------------------//
        //   compute the convergence rate    //
        //-----------------------------------//
        cr = r_norm / r_norm_old;
        
    } /* end of iteration while loop */
    
    if ( prtlvl > PRINT_NONE ) ITS_FINAL(iter,MaxIt,r_norm/den_norm);
    
FINISHED:
    /*-------------------------------------------
     * Free some stuff
     *------------------------------------------*/
    free(work);
    free(p);
    free(hh);
    free(norms);
    free(z);
    
    if ( iter >= MaxIt )
        return ERROR_SOLVER_MAXIT;
    else
        return iter;
}
