/*! \file src/nonlinear/newton.c
 *
 * \brief This code will contain all the tools needed to perform Newton stepping
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 10/18/16.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 * \note modified by James Adler 11/11/2016
 */

#include "hazmath.h"

/******************************************************************************************************/
/*!
 * \fn void initialize_newton(newton *n_it,input_param *inparam,INT ndof,INT blksize)
 *
 * \brief Initialize the Newton struct for nonlinear iterations.
 *
 * \param inparam       Input from input parameter list
 * \param ndof          Number of DOF in system
 * \param blksize       Block size of if block matrices (assumes square)
 *
 * \return n_it         Struct for Newton Iterations
 *
 */
void initialize_newton(newton *n_it,input_param *inparam,INT ndof,INT blksize)
{

  // Number of Newton steps
  n_it->max_steps = inparam->nonlinear_itsolver_maxit;

  // Current Step
  n_it->current_step = 0;

  // Tolerances 1 - ||nonlinear residual||<tol OR 2 - ||update|| < tol OR 0 - BOTH
  n_it->tol_type = inparam->nonlinear_itsolver_toltype;
  n_it->tol      = inparam->nonlinear_itsolver_tol;

  // Step Length: sol = sol_prev + step_length*update
  n_it->step_length = 1.0; // Default is full Newton

  // Matrices and Vectors
  // Assume the form A(sol) = f gets linearized to
  // Jacobian(sol_prev)[update] = f - A(sol_prev)
  if(n_it->isblock) {// In block form the Jacobian is a block_dCSRmat
    n_it->Jac=NULL;
    n_it->Jac_block=malloc(sizeof(struct block_dCSRmat));
    bdcsr_alloc(blksize,blksize,n_it->Jac_block);
  } else { //The Jacobian is a regular dCSRmat
    n_it->Jac=malloc(sizeof(struct dCSRmat));
    n_it->Jac_block=NULL;
  }
  n_it->sol=malloc(sizeof(struct dvector));
  n_it->sol_prev=malloc(sizeof(struct dvector));
  n_it->update=malloc(sizeof(struct dvector));
  n_it->rhs=malloc(sizeof(struct dvector));     /* f - A(sol_prev) */
  n_it->res_norm=0.0;
  n_it->update_norm=0.0;

  dvec_alloc(ndof,n_it->sol);
  dvec_alloc(ndof,n_it->rhs);

  return;
}
/******************************************************************************************************/

/****************************************************************************************/
/*!
 * \fn void free_newton(newton* n_it)
 *
 * \brief Frees memory of arrays of newton struct
 *
 * \return n_it         Freed struct for Newton Iterations
 *
 */
void free_newton(newton* n_it)
{
  if(n_it->Jac) {
    dcsr_free(n_it->Jac);
    free(n_it->Jac);
    n_it->Jac=NULL;
  }

  if(n_it->Jac_block) {
    bdcsr_free(n_it->Jac_block);
    free(n_it->Jac_block);
    n_it->Jac_block=NULL;
  }

  if(n_it->sol) {
    dvec_free(n_it->sol);
    free(n_it->sol);
    n_it->sol=NULL;
  }

  if(n_it->sol_prev) {
    dvec_free(n_it->sol_prev);
    free(n_it->sol_prev);
    n_it->sol_prev=NULL;
  }

  if(n_it->update) {
    dvec_free(n_it->update);
    free(n_it->update);
    n_it->update=NULL;
  }

  if(n_it->rhs) {
    dvec_free(n_it->rhs);
    free(n_it->rhs);
    n_it->rhs=NULL;
  }

  return;
}
/****************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn void update_newtonstep(newton* n_it)
 *
 * \brief Updates the Newton data at each step.
 *
 * \return n_it     Updated Newton struct
 *
 */
void update_newtonstep(newton* n_it)
{
  // Counters
  n_it->current_step++;

  // Solution
  if(n_it->current_step==1) {
    dvec_alloc(n_it->sol->row,n_it->sol_prev);
  }
  dvec_cp(n_it->sol,n_it->sol_prev);

  // Update
  if(n_it->current_step==1) {
    dvec_alloc(n_it->sol->row,n_it->update);
  }
  dvec_set(n_it->update->row,n_it->update,0.0);

  return;
}
/******************************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn void update_sol_newton(newton *n_it)
 *
 * \brief Updates the solution to the nonlinear problem.
 *        sol = sol_prev + step_length * update
 *
 * \return n_it.sol     Updated Newton solution
 *
 */
void update_sol_newton(newton *n_it)
{
  dvec_axpyz(n_it->step_length,n_it->update,n_it->sol_prev,n_it->sol);

  return;
}
/******************************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn INT check_newton_convergence(newton *n_it)
 *
 * \brief Checks if Newton has converged:
 *        If tol_type = 1: Check if ||nonlinear residual (rhs)|| < tol
 *                      2: Check if ||update|| < tol
 *                      0: Check both
 *
 * \param n_it     Newton struct
 *
 */
INT check_newton_convergence(newton *n_it)
{
  INT i;
  INT ndof=n_it->sol->row;
  REAL sum=0.0;
  INT newton_stop = 0;
  REAL tol = n_it->tol;

  // Get norms
  REAL res_norm = n_it->res_norm;
  REAL update_norm = n_it->update_norm;

  if(n_it->current_step>=n_it->max_steps) {
    newton_stop=1;
    printf("The Newton iterations have reached the max number of iterations (%d Newton Steps) \n",n_it->current_step);
    printf("Convergence may not be reached.\n");
    printf("Final Nonlinear Residual = %25.16e\tLast Update Norm = %25.16e\n",res_norm,update_norm);
    return newton_stop;
  }

  switch (n_it->tol_type)
  {
  default: // Compute ||uk - u_{k-1}||
    for(i=0;i<ndof;i++) {
      sum += (n_it->sol->val[i] - n_it->sol_prev->val[i])*(n_it->sol->val[i] - n_it->sol_prev->val[i]);
    }
    sum = sqrt(sum);
    printf("\n||uk-uk-1||=%25.16e\n",sum);
    if(sum<tol || res_norm<tol) {
      newton_stop=1;
      printf("Convergence met after %d Newton Steps.\n",n_it->current_step);
      printf("Final Nonlinear Residual = %25.16e\tLast Update Norm = %25.16e\t ||u_k - u_{k-1}||_l2 = %25.16e\n",res_norm,update_norm,sum);
    }
    break;
  case 1:
    if(res_norm<tol) {
      newton_stop=1;
      printf("Convergence met after %d Newton Steps.\n",n_it->current_step);
      printf("Final Nonlinear Residual = %25.16e\tLast Update Norm = %25.16e\n",res_norm,update_norm);
    }
    break;
  case 2:
    if(update_norm<tol) {
      newton_stop=1;
      printf("Convergence met after %d Newton Steps.\n",n_it->current_step);
      printf("Final Nonlinear Residual = %25.16e\tLast Update Norm = %25.16e\n",res_norm,update_norm);
    }
    break;
  case 3:
    if(res_norm<tol || update_norm<tol) {
      newton_stop=1;
      printf("Convergence met after %d Newton Steps.\n",n_it->current_step);
      printf("Final Nonlinear Residual = %25.16e\tLast Update Norm = %25.16e\n",res_norm,update_norm);
    }
    break;
  }

  return newton_stop;
}
/******************************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn void get_residual_norm(newton *n_it,fespace* FE,mesh_struct* mesh, qcoordinates* cq)
 *
 * \brief Computes the norms of the nonlinear residual (rhs) and the update.
 *
 * \param n_it     Newton struct
 * \param FE       FE space
 * \param mesh     Mesh struct
 * \param cq       Quadrature for computing norms
 *
 * \note Assumes non-block form
 */
void get_residual_norm(newton *n_it,fespace* FE,mesh_struct* mesh, qcoordinates* cq)
{
  n_it->res_norm = L2norm(n_it->rhs->val,FE,mesh,cq);
  return;
}
/******************************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn void get_update_norm(newton *n_it,fespace* FE,mesh_struct* mesh, qcoordinates* cq)
 *
 * \brief Computes the norms of the nonlinear residual (rhs) and the update.
 *
 * \param n_it     Newton struct
 * \param FE       FE space
 * \param mesh     Mesh struct
 * \param cq       Quadrature for computing norms
 *
 * \note Assumes non-block form
 */
void get_update_norm(newton *n_it,fespace* FE,mesh_struct* mesh, qcoordinates* cq)
{
  n_it->update_norm = L2norm(n_it->update->val,FE,mesh,cq);
  return;
}
/******************************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn void get_blockresidual_norm(newton *n_it,block_fespace* FE,mesh_struct* mesh, qcoordinates* cq)
 *
 * \brief Computes the norms of the nonlinear residual (rhs) and the update.
 *
 * \param n_it     Newton struct
 * \param FE       Block FE space
 * \param mesh     Mesh struct
 * \param cq       Quadrature for computing norms
 *
 * \note Assumes block form (and we store the total (combined) norm)
 */
void get_blockresidual_norm(newton *n_it,block_fespace* FE,mesh_struct* mesh, qcoordinates* cq)
{
  REAL* res_norm = (REAL *) calloc(FE->nspaces,sizeof(REAL));
  L2norm_block(res_norm,n_it->rhs->val,FE,mesh,cq);

  REAL total_res_norm = 0;
  INT i;
  for(i=0;i<FE->nspaces;i++) {
    total_res_norm += res_norm[i]*res_norm[i];
  }
  n_it->res_norm = sqrt(total_res_norm);

  if(res_norm) free(res_norm);
  return;
}
/******************************************************************************************************/

/******************************************************************************************************/
/*!
 * \fn void get_blockupdate_norm(newton *n_it,block_fespace* FE,mesh_struct* mesh, qcoordinates* cq)
 *
 * \brief Computes the norms of the nonlinear residual (rhs) and the update.
 *
 * \param n_it     Newton struct
 * \param FE       Block FE space
 * \param mesh     Mesh struct
 * \param cq       Quadrature for computing norms
 *
 * \note Assumes block form (and we store the total (combined) norm)
 */
void get_blockupdate_norm(newton *n_it,block_fespace* FE,mesh_struct* mesh, qcoordinates* cq)
{
  REAL* update_norm = (REAL *) calloc(FE->nspaces,sizeof(REAL));
  L2norm_block(update_norm,n_it->update->val,FE,mesh,cq);

  REAL total_update_norm = 0;
  INT i;
  for(i=0;i<FE->nspaces;i++) {
    total_update_norm += update_norm[i]*update_norm[i];
  }
  n_it->update_norm = sqrt(total_update_norm);

  if(update_norm) free(update_norm);
  return;
}
/******************************************************************************************************/



