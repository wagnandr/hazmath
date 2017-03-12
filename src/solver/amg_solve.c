/*! src/solver/amg_solve.c
 *
 *  Algebraic multigrid iterations: SOLVE phase.
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 12/24/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 * \note   Done cleanup for releasing -- Xiaozhe Hu 03/12/2017
 *
 */

#include "hazmath.h"
#include "itsolver_util.inl"

/*---------------------------------*/
/*--      Public Functions       --*/
/*---------------------------------*/
/**
 * \fn INT amg_solve (AMG_data *mgl, AMG_param *param)
 *
 * \brief Solve phase for AMG method (as standard alone iterative solver)
 *
 * \param mgl    Pointer to AMG data: AMG_data
 * \param param  Pointer to AMG parameters: AMG_param
 *
 * \return       Iteration number if converges; ERROR otherwise.
 *
 */
INT amg_solve(AMG_data *mgl,
              AMG_param *param)
{ 
    dCSRmat      *ptrA = &mgl[0].A;
    dvector      *b = &mgl[0].b, *x = &mgl[0].x, *r = &mgl[0].w;
    
    const SHORT   prtlvl = param->print_level;
    const INT     MaxIt  = param->maxit;
    const REAL    tol    = param->tol;
    const REAL    sumb   = dvec_norm2(b);
    
    // local variables
    REAL  solve_start, solve_end;
    REAL  relres1 = BIGREAL, absres0 = sumb, absres, factor;
    INT   iter = 0;

    get_time(&solve_start);
    
    // Print iteration information if needed
    print_itsolver_info(prtlvl, STOP_REL_RES, iter, 1.0, sumb, 0.0);
    
    // Main loop
    while ( (++iter <= MaxIt) & (sumb > SMALLREAL) ) {
        
        // Call one multigrid cycle
        mgcycle(mgl, param);
        
        // Form residual r = b - A*x
        dvec_cp(b, r);
        dcsr_aAxpy(-1.0, ptrA, x->val, r->val);
        
        // Compute norms of r and convergence factor
        absres  = dvec_norm2(r);
        relres1 = absres/sumb;
        factor  = absres/absres0;
        absres0 = absres;
        
        // Print iteration information if needed
        print_itsolver_info(prtlvl, STOP_REL_RES, iter, relres1, absres, factor);
        
        // Check convergence
        if ( relres1 < tol ) break;
    }
    
    if ( prtlvl > PRINT_NONE ) {
        ITS_FINAL(iter, MaxIt, relres1);
        get_time(&solve_end);
        print_cputime("AMG solve",solve_end - solve_start);
    }

    
    return iter;
}


/**
 * \fn INT amg_solve_amli (AMG_data *mgl, AMG_param *param)
 *
 * \brief Solve phase for AMG method using AMLI-cycle (as standard alone iterative solver)
 *
 * \param mgl    Pointer to AMG data: AMG_data
 * \param param  Pointer to AMG parameters: AMG_param
 *
 * \return       Iteration number if converges; ERROR otherwise.
 *
 * \author Xiaozhe Hu
 * \date   01/23/2011
 *
 * \note AMLI polynomial computed by the best approximation of 1/x.
 *       Refer to Johannes K. Kraus, Panayot S. Vassilevski,
 *       Ludmil T. Zikatanov, "Polynomial of best uniform approximation to $x^{-1}$
 *       and smoothing in two-level methods", 2013.
 *
 */
INT amg_solve_amli (AMG_data *mgl,
                         AMG_param *param)
{   
    dCSRmat     *ptrA = &mgl[0].A;
    dvector     *b = &mgl[0].b, *x = &mgl[0].x, *r = &mgl[0].w;
    
    const INT    MaxIt  = param->maxit;
    const SHORT  prtlvl = param->print_level;
    const REAL   tol    = param->tol;
    const REAL   sumb   = dvec_norm2(b); // L2norm(b)
    
    // local variables
    REAL         solve_start, solve_end, solve_time;
    REAL         relres1 = BIGREAL, absres0 = sumb, absres, factor;
    INT          iter = 0;
    
    get_time(&solve_start);

    // Print iteration information if needed
    print_itsolver_info(prtlvl, STOP_REL_RES, iter, 1.0, sumb, 0.0);
    
    // MG solver here
    while ( (++iter <= MaxIt) & (sumb > SMALLREAL) ) {
        
        // Call one AMLI cycle
        amli(mgl, param, 0);
        
        // Form residual r = b-A*x
        dvec_cp(b, r);
        dcsr_aAxpy(-1.0, ptrA, x->val, r->val);
        
        // Compute norms of r and convergence factor
        absres  = dvec_norm2(r);
        relres1 = absres/sumb;
        factor  = absres/absres0;
        absres0 = absres;
        
        // Print iteration information if needed
        print_itsolver_info(prtlvl, STOP_REL_RES, iter, relres1, absres, factor);
        
        // Check convergence
        if ( relres1 < tol ) break;
    }
    
    if ( prtlvl > PRINT_NONE ) {
        ITS_FINAL(iter, MaxIt, relres1);
        get_time(&solve_end);
        solve_time = solve_end - solve_start;
        print_cputime("AMLI solve", solve_time);
    }
    
    return iter;
}

/**
 * \fn INT amg_solve_nl_amli(AMG_data *mgl, AMG_param *param)
 *
 * \brief Solve phase for AMG method using nonlinear AMLI-cycle (as standard alone iterative solver)
 *
 * \param mgl    Pointer to AMG data: AMG_data
 * \param param  Pointer to AMG parameters: AMG_param
 *
 * \return       Iteration number if converges; ERROR otherwise.
 *
 * \author Xiaozhe Hu
 * \date   04/30/2011
 *
 * \note Nonlinear AMLI-cycle.
 *       Refer to Xiazhe Hu, Panayot S. Vassilevski, Jinchao Xu
 *       "Comparative Convergence Analysis of Nonlinear AMLI-cycle Multigrid", 2013.
 *
 */
INT amg_solve_nl_amli(AMG_data *mgl,
                      AMG_param *param)
{  
    dCSRmat      *ptrA = &mgl[0].A;
    dvector      *b = &mgl[0].b, *x = &mgl[0].x, *r = &mgl[0].w;
    
    const INT     MaxIt  = param->maxit;
    const SHORT   prtlvl = param->print_level;
    const REAL    tol    = param->tol;
    const REAL    sumb   = dvec_norm2(b); // L2norm(b)
    
    // local variables
    REAL          solve_start, solve_end;
    REAL          relres1 = BIGREAL, absres0 = BIGREAL, absres, factor;
    INT           iter = 0;
    
    get_time(&solve_start);
    
    // Print iteration information if needed
    print_itsolver_info(prtlvl, STOP_REL_RES, iter, 1.0, sumb, 0.0);
    
    while ( (++iter <= MaxIt) & (sumb > SMALLREAL) ) // MG solver here
    {
        // Call nonlinear AMLI-cycle
        nl_amli(mgl, param, 0, mgl[0].num_levels);
        
        // Computer r = b-A*x
        dvec_cp(b, r);
        dcsr_aAxpy(-1.0, ptrA, x->val, r->val);

        absres  = dvec_norm2(r); // residual ||r||
        relres1 = absres/sumb;       // relative residual ||r||/||b||
        factor  = absres/absres0;    // contraction factor
        absres0 = absres;


        // output iteration information if needed
        print_itsolver_info(prtlvl, STOP_REL_RES, iter, relres1, absres, factor);
        
        if ( relres1 < tol ) break; // early exit condition
    }
    
    if ( prtlvl > PRINT_NONE ) {
        ITS_FINAL(iter, MaxIt, relres1);
        get_time(&solve_end);
        print_cputime("Nonlinear AMLI solve", solve_end - solve_start);
    }
    
    return iter;
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/
