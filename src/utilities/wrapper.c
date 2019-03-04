/*! \file src/utilities/wrapper.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 09/02/16.
 *  Copyright 2016__HAZMATH__. All rights reserved.
 *
 *  \note: modified by Xiaozhe Hu on 10/27/2016
 *  \note: done cleanup for releasing -- Xiaozhe Hu 10/27/2016
 *
 */

#include "hazmath.h"

/*************************************************************************************/
/*!
 * \fn void python_wrapper_krylov_amg(INT *n, INT *nnz, INT *ia, INT *ja, REAL *a,
 *                                     REAL *b, REAL *u, REAL *tol, INT *maxit,
 *                                     INT *ptrlvl)
 *
 * \brief Solve Ax=b by Krylov method preconditioned by AMG (this is an interface with PYTHON)
 *
 * \param n             Number of cols of A
 * \param nnz           Number of nonzeros of A
 * \param ia            IA of A in CSR format
 * \param ja            JA of A in CSR format
 * \param a             VAL of A in CSR format
 * \param b             RHS vector
 * \param u             Solution vector
 * \param tol           Tolerance for iterative solvers
 * \param maxit         Max number of iterations
 * \param print_lvl     Print level for iterative solvers
 *
 * \author Xiaozhe Hu
 * \date   09/02/2016
 *
 */
void python_wrapper_krylov_amg(INT *n,
                               INT *nnz,
                               INT *ia,
                               INT *ja,
                               REAL *a,
                               REAL *b,
                               REAL *u,
                               REAL *tol,
                               INT *maxit,
                               INT *print_lvl)
{
    dCSRmat         mat;      // coefficient matrix
    dvector         rhs, sol; // right-hand-side, solution
    AMG_param       amgparam; // parameters for AMG
    linear_itsolver_param  itparam;  // parameters for linear itsolver

    input_param inparam;
    param_input_init(&inparam);
    param_input("./input.dat", &inparam);

    // Set parameters for linear iterative methods
    param_linear_solver_init(&itparam);
    param_linear_solver_set(&itparam, &inparam);
    if (*print_lvl > PRINT_MIN) param_linear_solver_print(&itparam);

    // Set parameters for algebriac multigrid methods
    param_amg_init(&amgparam);
    param_amg_set(&amgparam, &inparam);
    if (*print_lvl > PRINT_MIN) param_amg_print(&amgparam);

    amgparam.print_level          = *print_lvl;
    itparam.linear_tol            = *tol;
    itparam.linear_print_level    = *print_lvl;
    itparam.linear_maxit          = *maxit;

    mat.row = *n; mat.col = *n; mat.nnz = *nnz;
    mat.IA = ia;  mat.JA  = ja; mat.val = a;

    rhs.row = *n; rhs.val = b;
    sol.row = *n; sol.val = u;

    linear_solver_dcsr_krylov_amg(&mat, &rhs, &sol, &itparam, &amgparam);

}

/*!
 * \fn void python_wrapper_direct(INT *n, INT *nnz, INT *ia, INT *ja, REAL *a,
 *                                     REAL *b, REAL *u, INT *ptrlvl)
 *
 * \brief Solve Ax=b by direct method (this is an interface with PYTHON)
 *
 * \param n             Number of cols of A
 * \param nnz           Number of nonzeros of A
 * \param ia            IA of A in CSR format
 * \param ja            JA of A in CSR format
 * \param a             VAL of A in CSR format
 * \param b             RHS vector
 * \param u             Solution vector
 * \param print_lvl     Print level for iterative solvers
 *
 * \author Xiaozhe Hu
 * \date   08/03/2018
 *
 */
void python_wrapper_direct(INT *n,
                           INT *nnz,
                           INT *ia,
                           INT *ja,
                           REAL *a,
                           REAL *b,
                           REAL *u,
                           INT *print_lvl)
{
    dCSRmat         mat;      // coefficient matrix
    dvector         rhs, sol; // right-hand-side, solution

    mat.row = *n; mat.col = *n; mat.nnz = *nnz;
    mat.IA = ia;  mat.JA  = ja; mat.val = a;

    rhs.row = *n; rhs.val = b;
    sol.row = *n; sol.val = u;

    directsolve_UMF(&mat, &rhs, &sol, *print_lvl);
}


/*!
 * \fn void python_wrapper_krylov_block_2(INT *n, INT *nnz, INT *ia, INT *ja, REAL *a,
 *                                     REAL *b, REAL *u, REAL *tol, INT *maxit,
 *                                     INT *ptrlvl)
 *
 * \brief Solve Ax=b by Krylov method preconditioned in 2 by 2 block form (this is an interface with PYTHON)
 *
 * \param n             Number of cols of A
 * \param nnz           Number of nonzeros of A
 * \param ia            IA of A in CSR format
 * \param ja            JA of A in CSR format
 * \param a             VAL of A in CSR format
 * \param b             RHS vector
 * \param u             Solution vector
 * \param tol           Tolerance for iterative solvers
 * \param maxit         Max number of iterations
 * \param print_lvl     Print level for iterative solvers
 *
 * \author Xiaozhe Hu
 * \date   08/08/2018
 *
 */
void python_wrapper_krylov_block_2(INT *n,
                               INT *nnz,
                               INT *ia,
                               INT *ja,
                               REAL *a,
                               REAL *b,
                               REAL *u,
                               REAL *tol,
                               INT *maxit,
                               INT *print_lvl)
{
    dCSRmat         mat_csr;      // coefficient matrix in csr format
    dvector         rhs, sol; // right-hand-side, solution
    AMG_param       amgparam; // parameters for AMG
    linear_itsolver_param  itparam;  // parameters for linear itsolver

    input_param inparam;
    param_input_init(&inparam);
    param_input("./input.dat", &inparam);

    // Set parameters for linear iterative methods
    param_linear_solver_init(&itparam);
    param_linear_solver_set(&itparam, &inparam);
    if (*print_lvl > PRINT_MIN) param_linear_solver_print(&itparam);

    // Set parameters for algebriac multigrid methods
    param_amg_init(&amgparam);
    param_amg_set(&amgparam, &inparam);
    if (*print_lvl > PRINT_MIN) param_amg_print(&amgparam);

    amgparam.print_level          = *print_lvl;
    itparam.linear_tol            = *tol;
    itparam.linear_print_level    = *print_lvl;
    itparam.linear_maxit          = *maxit;

    // form CSR matrix
    mat_csr.row = *n; mat_csr.col = *n; mat_csr.nnz = *nnz;
    mat_csr.IA = ia;  mat_csr.JA  = ja; mat_csr.val = a;

    rhs.row = *n; rhs.val = b;
    sol.row = *n; sol.val = u;

    // convert into 2 by 2 block CSR matrix
    int *bsize;
    bsize = (int *)calloc(2, sizeof(int));
    bsize[0] = mat_csr.row/2; bsize[1] = mat_csr.row/2;

    block_dCSRmat mat_bdcsr = dcsr_2_bdcsr(&mat_csr, 2, bsize);

    // get diagonal blocks
    dCSRmat *mat_diag;
    mat_diag = (dCSRmat *)calloc(2, sizeof(dCSRmat));

    dcsr_alloc(mat_bdcsr.blocks[0]->row, mat_bdcsr.blocks[0]->col, mat_bdcsr.blocks[0]->nnz, &mat_diag[0]);
    dcsr_cp(mat_bdcsr.blocks[0], &mat_diag[0]);

    dcsr_alloc(mat_bdcsr.blocks[3]->row, mat_bdcsr.blocks[3]->col, mat_bdcsr.blocks[3]->nnz, &mat_diag[1]);
    dcsr_cp(mat_bdcsr.blocks[3], &mat_diag[1]);


    // solve in 2 by 2 block form
    linear_solver_bdcsr_krylov_block_2(&mat_bdcsr, &rhs, &sol, &itparam, &amgparam, mat_diag);

    // clean memory
    bdcsr_free(&mat_bdcsr);
    dcsr_free( &mat_diag[0]);
    dcsr_free( &mat_diag[1]);
    if(mat_diag) free(mat_diag);

}


/*!
 * \fn void python_wrapper_krylov_block_2by2(INT *n00, INT *nnz00, INT *ia00, INT *ja00, REAL *a00,
                                             INT *n01, INT *nnz01, INT *ia01, INT *ja01, REAL *a01,
                                             INT *n10, INT *nnz10, INT *ia10, INT *ja10, REAL *a10,
                                             INT *n11, INT *nnz11, INT *ia11, INT *ja11, REAL *a11
 *                                           REAL *b, REAL *u, REAL *tol, INT *maxit,
 *                                           INT *ptrlvl)
 *
 * \brief Solve Ax=b by Krylov method preconditioned in 2 by 2 block form (this is an interface with PYTHON)
 *
 * \param n00             Number of cols of A[0][0]
 * \param nnz00           Number of nonzeros of A[0][0]
 * \param ia00            IA of A[0][0] in CSR format
 * \param ja00            JA of A[0][0] in CSR format
 * \param a00             VAL of A[0][0] in CSR format
 * \param n01             Number of cols of A[0][1]
 * \param nnz01           Number of nonzeros of A[0][1]
 * \param ia01            IA of A[0][1] in CSR format
 * \param ja01            JA of A[0][1] in CSR format
 * \param a01             VAL of A[0][1] in CSR format
 * \param n10             Number of cols of A[1][0]
 * \param nnz10           Number of nonzeros of A[1][0]
 * \param ia10            IA of A[1][0] in CSR format
 * \param ja10            JA of A[1][0] in CSR format
 * \param a10             VAL of A[1][0] in CSR format
 * \param n11             Number of cols of A[1][1]
 * \param nnz11           Number of nonzeros of A[1][1]
 * \param ia11            IA of A[1][1] in CSR format
 * \param ja11            JA of A[1][1] in CSR format
 * \param a11             VAL of A[1][1] in CSR format
 * \param b             RHS vector
 * \param u             Solution vector
 * \param tol           Tolerance for iterative solvers
 * \param maxit         Max number of iterations
 * \param print_lvl     Print level for iterative solvers
 *
 * \author Xiaozhe Hu
 * \date   10/01/2018
 *
 */
void python_wrapper_krylov_block_2by2(INT *n00,
                                   INT *nnz00,
                                   INT *ia00,
                                   INT *ja00,
                                   REAL *a00,
                                   INT *n01,
                                   INT *nnz01,
                                   INT *ia01,
                                   INT *ja01,
                                   REAL *a01,
                                   INT *n10,
                                   INT *nnz10,
                                   INT *ia10,
                                   INT *ja10,
                                   REAL *a10,
                                   INT *n11,
                                   INT *nnz11,
                                   INT *ia11,
                                   INT *ja11,
                                   REAL *a11,
                                   REAL *b,
                                   REAL *u,
                                   REAL *tol,
                                   INT *maxit,
                                   INT *print_lvl)
{
    block_dCSRmat   mat_bdcsr;  // coefficient matrix in block CSR format
    dvector         rhs, sol; // right-hand-side, solution
    AMG_param       amgparam; // parameters for AMG
    linear_itsolver_param  itparam;  // parameters for linear itsolver

    input_param inparam;
    param_input_init(&inparam);
    param_input("./input.dat", &inparam);

    // Set parameters for linear iterative methods
    param_linear_solver_init(&itparam);
    param_linear_solver_set(&itparam, &inparam);
    if (*print_lvl > PRINT_MIN) param_linear_solver_print(&itparam);

    // Set parameters for algebriac multigrid methods
    param_amg_init(&amgparam);
    param_amg_set(&amgparam, &inparam);
    if (*print_lvl > PRINT_MIN) param_amg_print(&amgparam);

    amgparam.print_level          = *print_lvl;
    itparam.linear_tol            = *tol;
    itparam.linear_print_level    = *print_lvl;
    itparam.linear_maxit          = *maxit;

    // form block CSR matrix
    bdcsr_alloc(2, 2, &mat_bdcsr);
    // assgin 00 block
    mat_bdcsr.blocks[0]->row = *n00; mat_bdcsr.blocks[0]->col = *n00; mat_bdcsr.blocks[0]->nnz = *nnz00;
    mat_bdcsr.blocks[0]->IA = ia00; mat_bdcsr.blocks[0]->JA = ja00; mat_bdcsr.blocks[0]->val = a00;
    // assgin 01 block
    mat_bdcsr.blocks[1]->row = *n01; mat_bdcsr.blocks[1]->col = *n01; mat_bdcsr.blocks[1]->nnz = *nnz01;
    mat_bdcsr.blocks[1]->IA = ia01; mat_bdcsr.blocks[1]->JA = ja01; mat_bdcsr.blocks[1]->val = a01;
    // assgin 10 block
    mat_bdcsr.blocks[2]->row = *n10; mat_bdcsr.blocks[2]->col = *n10; mat_bdcsr.blocks[2]->nnz = *nnz10;
    mat_bdcsr.blocks[2]->IA = ia10; mat_bdcsr.blocks[2]->JA = ja10; mat_bdcsr.blocks[2]->val = a10;
    // assgin 11 block
    mat_bdcsr.blocks[3]->row = *n11; mat_bdcsr.blocks[3]->col = *n11; mat_bdcsr.blocks[3]->nnz = *nnz11;
    mat_bdcsr.blocks[3]->IA = ia11; mat_bdcsr.blocks[3]->JA = ja11; mat_bdcsr.blocks[3]->val = a11;

    //mat_csr.row = *n; mat_csr.col = *n; mat_csr.nnz = *nnz;
    //mat_csr.IA = ia;  mat_csr.JA  = ja; mat_csr.val = a;

    // form right hand side
    INT n = *n00 + *n11;
    rhs.row = n; rhs.val = b;
    sol.row = n; sol.val = u;

    // get diagonal blocks
    dCSRmat *mat_diag;
    mat_diag = (dCSRmat *)calloc(2, sizeof(dCSRmat));

    dcsr_alloc(mat_bdcsr.blocks[0]->row, mat_bdcsr.blocks[0]->col, mat_bdcsr.blocks[0]->nnz, &mat_diag[0]);
    dcsr_cp(mat_bdcsr.blocks[0], &mat_diag[0]);

    dcsr_alloc(mat_bdcsr.blocks[3]->row, mat_bdcsr.blocks[3]->col, mat_bdcsr.blocks[3]->nnz, &mat_diag[1]);
    dcsr_cp(mat_bdcsr.blocks[3], &mat_diag[1]);

    // solve in 2 by 2 block form
    linear_solver_bdcsr_krylov_block_2(&mat_bdcsr, &rhs, &sol, &itparam, &amgparam, mat_diag);

    // clean memory
    //bdcsr_free(&mat_bdcsr);
    dcsr_free( &mat_diag[0]);
    dcsr_free( &mat_diag[1]);
    if(mat_diag) free(mat_diag);

}

/*!
 * \fn void python_wrapper_krylov_mixed_darcy(INT *nrow00, INT *ncol00, INT *nnz00, INT *ia00, INT *ja00, REAL *a00,
                                        INT *nrow01, INT *ncol01, INT *nnz01, INT *ia01, INT *ja01, REAL *a01,
                                        INT *nrow10, INT *ncol10, INT *nnz10, INT *ia10, INT *ja10, REAL *a10,
                                        INT *nrow11, INT *ncol11, INT *nnz11, INT *ia11, INT *ja11, REAL *a11,
                                        REAL *b, REAL *u, REAL *tol, INT *maxit, INT *ptrlvl)
 *
 * \brief Solve Ax=b by Krylov method preconditioned in 2 by 2 block form (this is an interface with PYTHON)
 *
 * \param nrow00          Number of rows of A[0][0]
 * \param ncol00          Number of columns of A[0][0]
 * \param nnz00           Number of nonzeros of A[0][0]
 * \param ia00            IA of A[0][0] in CSR format
 * \param ja00            JA of A[0][0] in CSR format
 * \param a00             VAL of A[0][0] in CSR format
 * \param nrow01          Number of rows of A[0][1]
 * \param ncol01          Number of columns of A[0][1]
 * \param nnz01           Number of nonzeros of A[0][1]
 * \param ia01            IA of A[0][1] in CSR format
 * \param ja01            JA of A[0][1] in CSR format
 * \param a01             VAL of A[0][1] in CSR format
 * \param nrow10          Number of rows of A[1][0]
 * \param ncol10          Number of columns of A[1][0]
 * \param nnz10           Number of nonzeros of A[1][0]
 * \param ia10            IA of A[1][0] in CSR format
 * \param ja10            JA of A[1][0] in CSR format
 * \param a10             VAL of A[1][0] in CSR format
 * \param nrow11          Number of rows of A[1][1]
 * \param ncol11          Number of columns of A[1][1]
 * \param nnz11           Number of nonzeros of A[1][1]
 * \param ia11            IA of A[1][1] in CSR format
 * \param ja11            JA of A[1][1] in CSR format
 * \param a11             VAL of A[1][1] in CSR format
 * \param b             RHS vector
 * \param u             Solution vector
 * \param tol           Tolerance for iterative solvers
 * \param maxit         Max number of iterations
 * \param print_lvl     Print level for iterative solvers
 *
 * \author Xiaozhe Hu
 * \date   10/14/2018
 *
 */
 void python_wrapper_krylov_mixed_darcy(INT *nrow00,
                                  INT *ncol00,
                                  INT *nnz00,
                                  INT *ia00,
                                  INT *ja00,
                                  REAL *a00,
                                  INT *nrow01,
                                  INT *ncol01,
                                  INT *nnz01,
                                  INT *ia01,
                                  INT *ja01,
                                  REAL *a01,
                                  INT *nrow10,
                                  INT *ncol10,
                                  INT *nnz10,
                                  INT *ia10,
                                  INT *ja10,
                                  REAL *a10,
                                  INT *nrow11,
                                  INT *ncol11,
                                  INT *nnz11,
                                  INT *ia11,
                                  INT *ja11,
                                  REAL *a11,
                                  INT *nrowPidiv,
                                  INT *ncolPidiv,
                                  INT *nnzPidiv,
                                  INT *iaPidiv,
                                  INT *jaPidiv,
                                  REAL *aPidiv,
                                  INT *nrowCurl,
                                  INT *ncolCurl,
                                  INT *nnzCurl,
                                  INT *iaCurl,
                                  INT *jaCurl,
                                  REAL *aCurl,
                                  REAL *Mp_diag,
                                  REAL *b,
                                  REAL *u,
                                  REAL *tol,
                                  INT *maxit,
                                  INT *print_lvl,
                                  INT *iters)
 {
     block_dCSRmat   mat_bdcsr;  // coefficient matrix in block CSR format
     dvector         rhs, sol; // right-hand-side, solution
     AMG_param       amgparam; // parameters for AMG
     linear_itsolver_param  itparam;  // parameters for linear itsolver

     input_param inparam;
     param_input_init(&inparam);
     param_input("./input.dat", &inparam);

     // Set parameters for linear iterative methods
     param_linear_solver_init(&itparam);
     param_linear_solver_set(&itparam, &inparam);
     if (*print_lvl > PRINT_MIN) param_linear_solver_print(&itparam);

     // Set parameters for algebriac multigrid methods
     param_amg_init(&amgparam);
     param_amg_set(&amgparam, &inparam);
     if (*print_lvl > PRINT_MIN) param_amg_print(&amgparam);

     amgparam.print_level          = *print_lvl;
     itparam.linear_tol            = *tol;
     itparam.linear_print_level    = *print_lvl;
     itparam.linear_maxit          = *maxit;

     // form block CSR matrix
     bdcsr_alloc(2, 2, &mat_bdcsr);
     // assign 00 block
     mat_bdcsr.blocks[0]->row = *nrow00; mat_bdcsr.blocks[0]->col = *ncol00; mat_bdcsr.blocks[0]->nnz = *nnz00;
     mat_bdcsr.blocks[0]->IA = ia00; mat_bdcsr.blocks[0]->JA = ja00; mat_bdcsr.blocks[0]->val = a00;
     // assign 01 block
     mat_bdcsr.blocks[1]->row = *nrow01; mat_bdcsr.blocks[1]->col = *ncol01; mat_bdcsr.blocks[1]->nnz = *nnz01;
     mat_bdcsr.blocks[1]->IA = ia01; mat_bdcsr.blocks[1]->JA = ja01; mat_bdcsr.blocks[1]->val = a01;
     // assign 10 block
     mat_bdcsr.blocks[2]->row = *nrow10; mat_bdcsr.blocks[2]->col = *ncol10; mat_bdcsr.blocks[2]->nnz = *nnz10;
     mat_bdcsr.blocks[2]->IA = ia10; mat_bdcsr.blocks[2]->JA = ja10; mat_bdcsr.blocks[2]->val = a10;
     // assign 11 block
     mat_bdcsr.blocks[3]->row = *nrow11; mat_bdcsr.blocks[3]->col = *ncol11; mat_bdcsr.blocks[3]->nnz = *nnz11;
     mat_bdcsr.blocks[3]->IA = ia11; mat_bdcsr.blocks[3]->JA = ja11; mat_bdcsr.blocks[3]->val = a11;

     // form Pi_div and Curl matrices for HX preconditioner
     // assign Pidiv
     dCSRmat P_div;
     P_div.row = *nrowPidiv; P_div.col = *ncolPidiv; P_div.nnz = *nnzPidiv;
     P_div.IA = iaPidiv; P_div.JA = jaPidiv; P_div.val = aPidiv;
     // assign Curl
     dCSRmat Curl;
     Curl.row = *nrowCurl; Curl.col = *ncolCurl; Curl.nnz = *nnzCurl;
     Curl.IA = iaCurl; Curl.JA = jaCurl; Curl.val = aCurl;

     // form mass matrix of pressure (it is diagonal matrix, only diagonal is stored)
     dvector Mp;
     Mp.row = *nrow11; Mp.val = Mp_diag;

     // form right hand side
     INT n = *nrow00 + *nrow11;
     rhs.row = n; rhs.val = b;
     sol.row = n; sol.val = u;

     // solve in 2 by 2 block form
     *iters = linear_solver_bdcsr_krylov_mixed_darcy(&mat_bdcsr, &rhs, &sol, &itparam, &amgparam, &P_div, &Curl, NULL, &Mp);

     // clean memory
 }



/***************************** END ***************************************************/
