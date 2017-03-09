/*! \file amg_setup_ua.c
 *
 *  Created by James Adler and Xiaozhe Hu on 12/24/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *  ATTENTION: Do NOT use auto-indentation in this file!!!
 *
 */

#define SYMMETRIC_PAIRWISE 1 // use symmetric pairwise aggregation

/*---------------------------------*/
/*--      Private Functions      --*/
/*---------------------------------*/

#if 0

/***********************************************************************************************/
static INT krylov_cycle_dcsr_pgcg (dCSRmat *A,
                                        dvector *b,
                                        dvector *u,
                                        precond *pc)
{
    /**
     *  \fn int krylov_cycle_dcsr_pgcg (dCSRmat *A,  dvector *b, dvector *u, precond *pc)
     *
     *  \brief A preconditioned GCR method for solving Au=b
     *
     *  \param *A    Pointer to the coefficient matrix
     *  \param *b    Pointer to the dvector of right hand side
     *  \param *u    Pointer to the dvector of DOFs
     *  \param *pre  Pointer to the structure of precondition (precond)
     *
     * \author Zheng Li, Chensong Zhang
     * \date   11/09/2014
     *
     * \note   Specified for unsmoothed aggregation cycle
     *         Refer to YVAN NOTAY, "AN AGGREGATION-BASED ALGEBRAIC MULTIGRID METHOD", 2010
     */
    
    REAL   absres, relres, normb;
    REAL   alpha1, alpha2, gamma1, gamma2, rho1, rho2, beta1, beta2, beta3, beta4;
    REAL   *work, *r, *x1, *v1, *v2;
    
    INT    m=A->row;
    REAL   *x = u->val;
    
    // allocate temp memory
    work = (REAL *)calloc(4*m,sizeof(REAL));
    r = work; x1 = r + m; v1 = r + 2*m; v2 = r + 3*m;
    
    normb = array_norm2(m, b->val);
    
    array_cp(m, b->val, r);
    
    // Preconditioning
    if (pc != NULL)
        pc->fct(r, x, pc->data);
    else
        array_cp(m, r, x);
    
    //v1 = A*p
    dcsr_mxv(A, x, v1);
    
    // rho1 = (x,v1)
    rho1 = array_dotprod (m, x, v1);
    
    // alpha1 = (x, r)
    alpha1 = array_dotprod (m, x, r);
    
    beta1 = alpha1/rho1;
    
    // r = r - beta1 *v1
    array_axpy(m, -beta1, v1, r);
    
    // norm(r)
    absres = array_norm2(m, r);
    
    // compute relative residual
    relres = absres/normb;
    
    // if relres reaches tol(0.2), pgcg will stop,
    // otherwise, another one pgcg iteration will do.
    if (relres < 0.2) {
        array_ax(m, beta1, x);
        free(work);
        return SUCCESS;
    }
    
    // Preconditioning
    if (pc != NULL)
        pc->fct(r, x1, pc->data);
    else
        array_cp(m, r, x1);
    
    //v2 = A*x1
    dcsr_mxv(A, x1, v2);
    
    //gamma0 = (x1,v1)
    gamma1 = array_dotprod (m, x1, v1);
    
    //alpha2 = (x1,r)
    alpha2  = array_dotprod(m, x1, r);
    
    //rho2 = (x1,v2)
    rho2 = array_dotprod(m, x1, v2);
    
    gamma2 = gamma1;
    //gamma2 = array_dotprod(m, x, v2);
    
    beta2 = rho2 - gamma1*gamma2/rho1;
    beta3 = (alpha1 - gamma2*alpha2/beta2)/rho1;
    beta4 = alpha2/beta2;
    
    array_ax(m, beta3, x);
    
    array_axpy(m, beta4, x1, x);
    
    // free
    free(work);
    
    return SUCCESS;
}



static void pairwise_aggregation_initial (const dCSRmat *A,
                                          INT checkdd,
                                          REAL kaptg,
                                          INT *iperm,
                                          ivector *vertices,
                                          REAL *s)
{
    /**
     * \fn static void pairwise_aggregation_initial (const dCSRmat *A,
     *                                               INT checkdd,
     *                                               INT *iperm,
     *                                               ivector *vertices,
     *                                               REAL *s)
     *
     * \brief Initial vertices for first pass aggregation
     *
     * \param A         Pointer to the coefficient matrices
     * \param checkdd   Pointer to diagonal dominant checking
     * \param iperm     Pointer to large positive off-diagonal rows.
     * \param vertices  Pointer to the aggregation of vertices
     * \param s         Pointer to off-diagonal row sum
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     */
    
    
    INT i, j, col;
    INT row = A->row;
    INT *ia = A->IA;
    INT *ja = A->JA;
    REAL *val = A->val;
    REAL strong_hold, aij, aii, rowsum, absrowsum, max;
    
    REAL *colsum = (REAL*)calloc(row, sizeof(REAL));
    REAL *colmax = (REAL*)calloc(row, sizeof(REAL));
    REAL *abscolsum = (REAL*)calloc(row, sizeof(REAL));
    
    strong_hold = kaptg/(kaptg - 2.0);
    
    for (i=0; i<row; ++i) {
        for (j=ia[i]+1; j<ia[i+1]; ++j) {
            col = ja[j];
            aij = val[j];
            colsum[col] += aij;
            colmax[col] = MAX(colmax[col], aij);
            if (checkdd) abscolsum[col] += ABS(aij);
        }
    }
    
    for (i=0; i<row; ++i) {
        rowsum = 0.0, max = 0.0, absrowsum = 0.0;
        aii = val[ia[i]];
        for (j=ia[i]+1; j<ia[i+1]; ++j) {
            aij = val[j];
            rowsum += aij;
            max = MAX(max, aij);
            if (checkdd) absrowsum += ABS(aij);
        }
        rowsum = 0.5*(colsum[i] + rowsum);
        max = MAX(colmax[i], max);
        if (checkdd) absrowsum = 0.5*(abscolsum[i] + absrowsum);
        
        s[i] = -rowsum;
        
        if (aii > strong_hold*absrowsum) {
            vertices->val[i] = G0PT;
        }
        else {
            vertices->val[i] = UNPT;
            if (max > 0.45*aii) iperm[i] = -1;
        }
    }
    
    free(colsum);
    free(colmax);
    free(abscolsum);
}

static void pairwise_aggregation_initial2 (const dCSRmat *A,
                                           ivector *vertices,
                                           REAL *s)
{
    /**
     * \fn static void pairwise_aggregation_initial2 (const dCSRmat *A,
     *                                                ivector *vertices,
     *                                                REAL *s)
     *
     * \brief Initial vertices for second pass aggregation based on temporary coarse
     *        matrix
     *
     * \param A         Pointer to the coefficient matrices
     * \param vertices  Pointer to the aggregation of vertices
     * \param s         Pointer to off-diagonal row sum
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     */
    
    INT i, j, col;
    INT row = A->row;
    INT *ia = A->IA;
    INT *ja = A->JA;
    REAL *val = A->val;
    REAL aij, rowsum;
    
    REAL *colsum = (REAL*)calloc(row, sizeof(REAL));
    
    for (i=0; i<row; ++i) {
        for (j=ia[i]+1; j<ia[i+1]; ++j) {
            col = ja[j];
            aij = val[j];
            colsum[col] += aij;
        }
    }
    
    for (i=0; i<row; ++i) {
        rowsum = 0.0;
        for (j=ia[i]+1; j<ia[i+1]; ++j) {
            aij = val[j];
            rowsum += aij;
        }
        rowsum = 0.5*(colsum[i] + rowsum);
        s[i] = -rowsum;
        vertices->val[i] = UNPT;
    }
    
    free(colsum);
}


static void pairwise_aggregation_initial3 (const dCSRmat *A,
                                           ivector *map,
                                           ivector *vertice,
                                           REAL *s1,
                                           REAL *s)
{
    /**
     * \fn static void pairwise_aggregation_initial3 (const dCSRmat *A,
     *                                                ivector *map,
     *                                                ivector *vertices,
     *                                                REAL *s1,
     *                                                REAL *s)
     *
     * \brief Initial vertices for second pass aggregation based on initial matrix
     *
     * \param A         Pointer to the coefficient matrices
     * \param map       Pointer to mapping form fine nodes to coarse nodes
     * \param vertices  Pointer to the aggregation of vertices
     * \param s1        Pointer to off-diagonal row sum of initial matrix
     * \param s         Pointer to off-diagonal row sum of temporary coarse matrix
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     */
    
    INT i, j, k, col, nc;
    INT *ia = A->IA;
    INT *ja = A->JA;
    REAL *val = A->val;
    
    REAL si;
    INT num_agg = map->row/2;
    
    for (i=0; i<num_agg; ++i) {
        j = map->val[2*i];
        si = 0;
        si = si + s1[j];
        for (k=ia[j]; k<ia[j+1]; ++k) {
            col = ja[k];
            nc = vertice->val[col];
            if ((nc==i) && (col != j)) si += val[k];
        }
        j = map->val[2*i+1];
        if (j < 0) {
            s[i] = si;
            continue;
        }
        si = si + s1[j];
        for (k=ia[j]; k<ia[j+1]; ++k) {
            col = ja[k];
            nc = vertice->val[col];
            if ((nc==i) && (col != j)) si += val[k];
        }
        s[i] = si;
    }
}


static INT cholesky_factorization_check (REAL W[8][8],
                                         INT agg_size)
{
    
    /**
     * \fn static INT cholesky_factorization_check (REAL W[8][8],
     *                                              INT agg_size)
     *
     * \brief Cholesky factorization
     *
     * \param W        Pointer to the coefficient matrices
     * \param agg_size Size of aggregate
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     */
    
    REAL T;
    INT status = 0;
    
    if (agg_size >= 8) {
        if (W[7][7] <= 0.0) return status;
        W[6][6] = W[6][6] - (W[6][7]/W[7][7]) * W[6][7];
        T = W[4][6]/W[6][6];
        W[5][6] = W[5][6] - T * W[6][7];
        W[5][5] = W[5][5] - T * W[5][7];
        T = W[4][7]/W[7][7];
        W[4][6] = W[4][6] - T * W[6][7];
        W[4][5] = W[4][5] - T * W[5][7];
        W[4][4] = W[4][4] - T * W[4][7];
        T = W[3][7]/W[7][7];
        W[3][6] = W[3][6] - T * W[6][7];
        W[3][5] = W[3][5] - T * W[5][7];
        W[3][4] = W[3][4] - T * W[4][7];
        W[3][3] = W[3][3] - T * W[3][7];
        T = W[2][7]/W[7][7];
        W[2][6] = W[2][6] - T * W[6][7];
        W[2][5] = W[2][5] - T * W[5][7];
        W[3][5] = W[3][5] - T * W[4][7];
        W[2][3] = W[2][3] - T * W[3][7];
        W[2][2] = W[2][2] - T * W[2][7];
        T = W[1][7]/W[7][7];
        W[1][6] = W[1][6] - T * W[6][7];
        W[1][5] = W[1][5] - T * W[5][7];
        W[1][4] = W[1][4] - T * W[4][7];
        W[1][3] = W[1][3] - T * W[3][7];
        W[1][2] = W[1][2] - T * W[2][7];
        W[1][1] = W[1][1] - T * W[1][7];
        T = W[0][7]/W[7][7];
        W[0][6] = W[0][6] - T * W[6][7];
        W[0][5] = W[0][5] - T * W[5][7];
        W[0][4] = W[0][4] - T * W[4][7];
        W[0][3] = W[0][3] - T * W[3][7];
        W[0][2] = W[0][2] - T * W[2][7];
        W[0][1] = W[0][1] - T * W[1][7];
        W[0][0] = W[0][0] - T * W[0][7];
    }
    if (agg_size >= 7) {
        if (W[6][6] <= 0.0) return status;
        W[5][5] = W[5][5] - (W[5][6]/W[6][6]) * W[5][6];
        T = W[4][6]/W[6][6];
        W[4][5] = W[4][5] - T * W[5][6];
        W[4][4] = W[4][4] - T * W[4][6];
        T = W[3][6]/W[6][6];
        W[3][5] = W[3][5] - T * W[5][6];
        W[3][4] = W[3][4] - T * W[4][6];
        W[3][3] = W[3][3] - T * W[3][6];
        T = W[2][6]/W[6][6];
        W[2][5] = W[2][5] - T * W[5][6];
        W[3][5] = W[3][5] - T * W[4][6];
        W[2][3] = W[2][3] - T * W[3][6];
        W[2][2] = W[2][2] - T * W[2][6];
        T = W[1][6]/W[6][6];
        W[1][5] = W[1][5] - T * W[5][6];
        W[1][4] = W[1][4] - T * W[4][6];
        W[1][3] = W[1][3] - T * W[3][6];
        W[1][2] = W[1][2] - T * W[2][6];
        W[1][1] = W[1][1] - T * W[1][6];
        T = W[0][6]/W[6][6];
        W[0][5] = W[0][5] - T * W[5][6];
        W[0][4] = W[0][4] - T * W[4][6];
        W[0][3] = W[0][3] - T * W[3][6];
        W[0][2] = W[0][2] - T * W[2][6];
        W[0][1] = W[0][1] - T * W[1][6];
        W[0][0] = W[0][0] - T * W[0][6];
    }
    if (agg_size >= 6) {
        if (W[5][5] <= 0.0) return status;
        W[4][4] = W[4][4] - (W[4][5]/W[5][5]) * W[4][5];
        T = W[3][5]/W[5][5];
        W[3][4] = W[3][4] - T * W[4][5];
        W[3][3] = W[3][3] - T * W[3][5];
        T = W[2][5]/W[5][5];
        W[3][5] = W[3][5] - T * W[4][5];
        W[2][3] = W[2][3] - T * W[3][5];
        W[2][2] = W[2][2] - T * W[2][5];
        T = W[1][5]/W[5][5];
        W[1][4] = W[1][4] - T * W[4][5];
        W[1][3] = W[1][3] - T * W[3][5];
        W[1][2] = W[1][2] - T * W[2][5];
        W[1][1] = W[1][1] - T * W[1][5];
        T = W[0][5]/W[5][5];
        W[0][4] = W[0][4] - T * W[4][5];
        W[0][3] = W[0][3] - T * W[3][5];
        W[0][2] = W[0][2] - T * W[2][5];
        W[0][1] = W[0][1] - T * W[1][5];
        W[0][0] = W[0][0] - T * W[0][5];
    }
    if (agg_size >= 5) {
        if (W[4][4] <= 0.0) return status;
        W[3][3] = W[3][3] - (W[3][4]/W[4][4]) * W[3][4];
        T = W[3][5]/W[4][4];
        W[2][3] = W[2][3] - T * W[3][4];
        W[2][2] = W[2][2] - T * W[3][5];
        T = W[1][4]/W[4][4];
        W[1][3] = W[1][3] - T * W[3][4];
        W[1][2] = W[1][2] - T * W[3][5];
        W[1][1] = W[1][1] - T * W[1][4];
        T = W[0][4]/W[4][4];
        W[0][3] = W[0][3] - T * W[3][4];
        W[0][2] = W[0][2] - T * W[3][5];
        W[0][1] = W[0][1] - T * W[1][4];
        W[0][0] = W[0][0] - T * W[0][4];
    }
    if (agg_size >= 4) {
        if (W[3][3] <= 0.0) return status;
        W[2][2] = W[2][2] - (W[2][3]/W[3][3]) * W[2][3];
        T = W[1][3]/W[3][3];
        W[1][2] = W[1][2] - T * W[2][3];
        W[1][1] = W[1][1] - T * W[1][3];
        T = W[0][3]/W[3][3];
        W[0][2] = W[0][2] - T * W[2][3];
        W[0][1] = W[0][1] - T * W[1][3];
        W[0][0] = W[0][0] - T * W[0][3];
    }
    if (agg_size >= 3) {
        if (W[2][2] <= 0.0) return status;
        W[1][1] = W[1][1] - (W[1][2]/W[2][2]) * W[1][2];
        T = W[0][2]/W[2][2];
        W[0][1] = W[0][1] - T * W[1][2];
        W[0][0] = W[0][0] - T * W[0][2];
    }
    if (agg_size >= 2) {
        if (W[1][1] <= 0.0) return status;
        W[0][0] = W[0][0] - (W[0][1]/W[1][1]) * W[0][1];
    }
    if (agg_size >= 1) {
        if (W[0][0] <= 0.0) return status;
    }
    
    status = 1;
    return status;
}


static INT aggregation_quality_check(dCSRmat *A,
                                     ivector *tentmap,
                                     REAL *s,
                                     INT root,
                                     INT pair,
                                     INT dopass,
                                     REAL quality_bound)
{
    /**
     * \fn static INT aggregation_quality_check (dCSRmat *A,
     *                                          ivector *tentmap,
     *                                          REAL *s,
     *                                          INT root,
     *                                          INT pair,
     *                                          INT dopass)
     *
     * \brief From local matrix corresponding to each aggregate
     *
     * \param A                 Pointer to the coefficient matrices
     * \param tentmap           Pointer to the map of first pass
     * \param s                 Pointer to off-diagonal row sum
     * \param root              Root node of each aggregate
     * \param pair              Associate node of each aggregate
     * \param dopass            Number of pass
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     *
     * Use a similar method as in AGMG; refer to Yvan Notay's AGMG-3.2.0.
     */
    
    INT *IA = A->IA;
    INT *JA = A->JA;
    REAL *val = A->val;
    INT *map = tentmap->val;
    
    REAL bnd1=2*1.0/quality_bound;
    REAL bnd2=1.0-bnd1;
    
    REAL W[8][8], v[4], sig[4], AG[4];
    INT fnode[4];
    
    REAL alpha, beta;
    INT i, j, l, k, m, status, flag, jj, agg_size;
    
    if (dopass == 2) {
        if (map[2*root+1] < -1) {
            if (map[2*pair+1] < -1) {
                fnode[0] = map[2*root];
                fnode[1] = map[2*pair];
                agg_size = 2;
            }
            else {
                fnode[0] = map[2*root];
                fnode[1] = map[2*pair];
                fnode[2] = map[2*pair+1];
                agg_size = 3;
            }
        }
        else {
            if (map[2*pair+1] < -1) {
                fnode[0] = map[2*root];
                fnode[1] = map[2*root+1];
                fnode[2] = map[2*pair];
                agg_size = 3;
            }
            else {
                fnode[0] = map[2*root];
                fnode[1] = map[2*root+1];
                fnode[2] = map[2*pair];
                fnode[3] = map[2*pair+1];
                agg_size = 4;
            }
        }
    }
    
    flag = 1;
    
    while (flag) {
        flag = 0;
        for(i=1; i<agg_size; ++i) {
            if (fnode[i] < fnode[i-1]) {
                jj = fnode[i];
                fnode[i] = fnode[i-1];
                fnode[i-1] = jj;
                flag = 1;
            }
        }
    }
    
    for (j=0; j<agg_size; ++j) {
        jj = fnode[j];
        sig[j] = s[jj];
        W[j][j]= val[IA[jj]];
        AG[j]  = W[j][j]-sig[j];
        for (l=j+1; l<agg_size; ++l) {
            W[j][l]=0.0;
            W[l][j]=0.0;
        }
        
        for (k=IA[jj]; k<IA[jj+1]; ++k) {
            if (JA[k]>jj)
                for (l=j+1; l<agg_size; ++l) {
                    m = fnode[l];
                    if (JA[k]==m) {
                        alpha=val[k]/2;
                        W[j][l]=alpha;
                        W[l][j]=alpha;
                        break;
                    }
                }
        }
        
        for (k=IA[jj]; k<IA[jj+1]; ++k) {
            if (JA[k] < jj)
                for (l=0; l<j; ++l) {
                    m = fnode[l];
                    if (JA[k] == m) {
                        alpha = val[k]/2;
                        W[j][l] = W[j][l]+alpha;
                        W[l][j] = W[j][l];
                        break;
                    }
                }
        }
    }
    
    for (j=0; j<agg_size; ++j) {
        for (k=0; k<agg_size; ++k) {
            if (j != k) sig[j] += W[j][k];
        }
        if (sig[j] < 0.0)  AG[j] = AG[j] + 2*sig[j];
        v[j] = W[j][j];
        W[j][j] = bnd2*W[j][j]-ABS(sig[j]);
        
        if (j == 0) {
            beta = v[j];
            alpha = ABS(AG[j]);
        }
        else {
            beta = beta + v[j];
            alpha = MAX(alpha,ABS(AG[j]));
        }
    }
    beta = bnd1/beta;
    
    for (j=0; j<agg_size; ++j) {
        for (k=0; k<agg_size; ++k) {
            W[j][k] = W[j][k] + beta*v[j]*v[k];
        }
    }
    
    if (alpha < 1.5e-8*beta) {
        agg_size --;
    }
    
    status = cholesky_factorization_check(W, agg_size);
    
    return status;
}



static void first_pairwise_unsymm (const dCSRmat * A,
                                   const REAL    k_tg,
                                   INT           * order,
                                   ivector       * vertices,
                                   ivector       * map,
                                   REAL          * s,
                                   INT           * num_agg)
{
    /**
     * \fn void first_pairwise_unsymm (const dCSRmat * A,
     *                                 INT    *order,
     *                                 ivector *vertices,
     *                                 ivector *map,
     *                                 REAL    *s,
     *                                 INT *num_agg)
     *
     * \brief Form initial pass aggregation for non-symmetric problem
     *
     * \param A          Pointer to the coefficient matrices
     * \param order      Pointer to the order of nodes
     * \param vertices   Pointer to the aggregation of vertices
     * \param map        Pointer to the map index of fine nodes to coarse nodes
     * \param s          Pointer to off-diagonal row sum
     * \param num_agg    Pointer to number of aggregations
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     *
     * \note  Refer to Yvan Notay "Aggregation-based algebraic multigrid
     *        for convection-diffusion equations" 2011.
     */
    
    
    const INT row  = A->row;
    INT  *AIA  = A->IA;
    INT  *AJA  = A->JA;
    REAL *Aval = A->val;
    //const REAL k_tg = 10.0;
    
    INT i, j, row_start, row_end, nc, col, k, ipair, node, checkdd;
    REAL mu, aii, ajj, aij, aji, tent, vals, val;
    REAL del1, del2, eta1, eta2, sig1, sig2, rsi, rsj, epsr,del12;
    
    nc = 0;
    i = 0;
    node = 0;
    checkdd = 1;
    
    /*---------------------------------------------------------*/
    /* Step 1. select extremely strong diagonal dominate rows  */
    /*        and store in G0.                                 */
    /*---------------------------------------------------------*/
    
    /* G0 : vertices->val[i]=G0PT, Remain: vertices->val[i]=UNPT */
    
    ivec_alloc(row, vertices);
    ivec_alloc(2*row, map);
    
    INT *iperm = (INT *)calloc(row, sizeof(INT));
    
    /*---------------------------------------------------------*/
    /* Step 2. compute row sum (off-diagonal) for each vertex  */
    /*---------------------------------------------------------*/
    pairwise_aggregation_initial(A, checkdd, k_tg, iperm, vertices, s);
    
    /*-----------------------------------------*/
    /* Step 3. start the pairwise aggregation  */
    /*-----------------------------------------*/
    while (node < row) {
        
        // deal with G0 type node
        if ( vertices->val[i] == G0PT ) {
            node ++;
            i ++;
            continue;
        }
        // check nodes whether are aggregated
        if ( vertices->val[i] != UNPT ) { i ++ ; continue;}
        
        vertices->val[i] = nc;
        map->val[2*nc] = i;
        node ++;
        
        // check whether node has large off-diagonal positive node or not
        if ( iperm[i] == -1 ) {
            map->val[2*nc+1] = -1;
            nc ++;
            i ++;
            continue;
        }
        
        ipair = -1;
        
        row_start = AIA[i]; row_end = AIA[i+1];
        
        aii = Aval[row_start];
        
        for ( j= row_start + 1; j < row_end; j++ ) {
            col = AJA[j];
            if ( vertices->val[col] != UNPT || iperm[col] == -1) continue;
            
            aij = Aval[j];
            ajj = Aval[AIA[col]];
            aji = 0.0;
            
            for(k = AIA[col]; k < AIA[col+1]; ++k) {
                if (AJA[k] == i) {
                    aji = Aval[k];
                    break;
                }
            }
            
            vals = -0.5*(aij+aji);
            
            rsi = -s[i] + aii;
            rsj = -s[col] + ajj;
            
            eta1 = 2*aii;
            eta2 = 2*ajj;
            
            sig1 = s[i]-vals;
            sig2 = s[col]-vals;
            
            if (sig1 > 0) {
                del1 = rsi;
            } else {
                del1 = rsi+2*sig1;
            }
            if (sig2 > 0) {
                del2 = rsj;
            } else {
                del2 = rsj+2*sig2;
            }
            if (vals > 0.0) {
                epsr=1.49e-8*vals;
                if ((ABS(del1) < epsr) && (ABS(del2) < epsr)) {
                    mu = (eta1*eta2)/(vals*(eta1+eta2));
                } else if (ABS(del1) < epsr) {
                    if(del2 < -epsr) continue;
                    mu = (eta1*eta2)/(vals*(eta1+eta2));
                } else if (ABS(del2) < epsr) {
                    if (del1 < -epsr) continue;
                    mu = (eta1*eta2)/(vals*(eta1+eta2));
                } else {
                    del12 = del1 + del2;
                    if (del12 < -epsr) continue;
                    mu = vals + del1*del2/del12;
                    if (mu < 0.0) continue;
                    mu = ((eta1*eta2)/(eta1+eta2))/mu;
                }
            }
            else {
                if (del1 <= 0.0 || del2 <= 0.0) continue;
                mu = vals + del1*del2/(del1+del2);
                if (mu < 0.0) continue;
                vals = (eta1*eta2)/(eta1+eta2);
                mu = vals/mu;
            }
            
            if (mu > k_tg) continue;
            
            tent = mu;
            
            if (ipair == -1) {
                ipair = col;
                val = tent;
            }
            else if((tent-val) < -0.06){
                ipair = col;
                val = tent;
            }
        }
        
        if ( ipair == -1) {
            map->val[2*nc+1] = -2;
        }
        else {
            vertices->val[ipair] = nc;
            map->val[2*nc+1] = ipair;
            node ++;
        }
        
        nc++;
        i ++;
    }
    
    map->row = 2*nc;
    map->val = (INT*)realloc(map->val, sizeof(INT)*map->row);
    
    *num_agg = nc;
    
    free(iperm);
}


static void second_pairwise_unsymm (dCSRmat *A,
                                    dCSRmat *tmpA,
                                    const REAL k_tg,
                                    INT dopass,
                                    INT *order,
                                    ivector *map1,
                                    ivector *vertices1,
                                    ivector *vertices,
                                    ivector *map,
                                    REAL *s1,
                                    INT *num_agg)
{
    /**
     * \fn void second_pairwise_unsymm(dCSRmat *A,
     *                                 dCSRmat *tmpA,
     *                                 INT dopass,
     *                                 INT *order,
     *                                 ivector *map1,
     *                                 ivector *vertices1,
     *                                 ivector *vertices,
     *                                 ivector *map,
     *                                 REAL *s1,
     *                                 INT *num_agg)
     *
     * \brief Form second pass aggregation for non-symmetric problem
     *
     * \param A          Pointer to the coefficient matrices
     * \param tmpA       Pointer to the first pass aggregation coarse matrix
     * \param dopass     Pointer to the number of pass
     * \param order      Pointer to the order of nodes
     * \param map1       Pointer to the map index of fine nodes to coarse nodes in
     *                   initial pass
     * \param vertices1  Pointer to the aggregation of vertices in initial pass
     * \param vertices   Pointer to the aggregation of vertices in the second pass
     * \param map        Pointer to the map index of fine nodes to coarse nodes in
     *                   the second pass
     * \param s1         Pointer to off-diagonal row sum of matrix
     * \param num_agg    Pointer to number of aggregations
     *
     * \author Zheng Li, Chensong Zhang
     * \date   12/23/2014
     *
     * \note  Refer to Yvan Notay "Aggregation-based algebraic multigrid
     *        for convection-diffusion equations" 2011.
     */
    
    
    INT i, j, k, l, m, ijtent;
    INT row = tmpA->row;
    INT *AIA = tmpA->IA;
    INT *AJA = tmpA->JA;
    REAL *Aval = tmpA->val;
    
    //const REAL k_tg = 10.0 ;
    
    REAL *Tval;
    INT *Tnode;
    
    INT  col,ipair,Tsize, row_start, row_end, Semipd, nc, node;
    REAL mu, aii, ajj, aij, tmp, val, aji, vals;
    REAL del1, del2, eta1, eta2, sig1, sig2, rsi, rsj, epsr,del12;
    
    Tval  = (REAL*)calloc(row, sizeof(REAL));
    Tnode = (INT*)calloc(row, sizeof(INT));
    
    ivec_alloc(2*row, map);
    ivec_alloc(row, vertices);
    
    nc = node = 0;
    
    REAL *s = (REAL *)calloc(row, sizeof(REAL));
    
    //pairwise_aggregation_initial2(tmpA, vertices, s);
    pairwise_aggregation_initial3(A, map1, vertices1, s1, s);
    
    ivec_set(UNPT, vertices);
    
    i = 0;
    
    while (node < row) {

        // check nodes whether are aggregated
        if ( vertices->val[i] != UNPT ) {
            i++;
            continue;
        }
        
        vertices->val[i] = nc;
        map->val[2*nc] = i;
        
        node ++;
        // if node isolated in first pass will be isolated in second pass
        if (map1->val[2*i+1] == -1) {
            map->val[2*nc+1] = -1;
            nc ++;
            i ++;
            continue;
        }
        
        ipair = -1;
        Tsize = 0;
        
        row_start = AIA[i]; row_end = AIA[i+1];
        
        aii = Aval[row_start];
        
        for ( j= row_start + 1; j < row_end; j++ ) {
            col = AJA[j];
            
            if ( vertices->val[col] != UNPT || map1->val[2*col+1] == -1) continue;
            
            aji = 0.0;
            aij = Aval[j];
            ajj = Aval[AIA[col]];
            
            for (k = AIA[col]; k < AIA[col+1]; ++k) {
                if (AJA[k]==i) {
                    aji = Aval[k];
                    break;
                }
            }
            
            vals = -0.5*(aij+aji);
            rsi = -s[i] + aii;
            rsj = -s[col] + ajj;
            eta1 = 2*aii;
            eta2 = 2*ajj;
            
            sig1 = s[i]-vals;
            sig2 = s[col]-vals;
            
            if (sig1 > 0) {
                del1 = rsi;
            } else {
                del1 = rsi+2*sig1;
            }
            if (sig2 > 0) {
                del2 = rsj;
            } else {
                del2 = rsj+2*sig2;
            }
            if (vals > 0.0) {
                epsr=1.49e-8*vals;
                if ((ABS(del1) < epsr) && (ABS(del2) < epsr)) {
                    mu = (eta1*eta2)/(vals*(eta1+eta2));
                } else if (ABS(del1) < epsr) {
                    if(del2 < -epsr) continue;
                    mu = (eta1*eta2)/(vals*(eta1+eta2));
                } else if (ABS(del2) < epsr) {
                    if (del1 < -epsr) continue;
                    mu = (eta1*eta2)/(vals*(eta1+eta2));
                } else {
                    del12 = del1 + del2;
                    if (del12 < -epsr) continue;
                    mu = vals + del1*del2/del12;
                    if (mu < 0.0) continue;
                    mu = ((eta1*eta2)/(eta1+eta2))/mu;
                }
            }
            else {
                if (del1 <= 0.0 || del2 <= 0.0) continue;
                mu = vals + del1*del2/(del1+del2);
                if (mu < 0.0) continue;
                aij = (eta1*eta2)/(eta1+eta2);
                mu = aij/mu;
            }
            if (mu > k_tg) continue;
            
            tmp = mu;
            
            if (ipair == -1) {
                ipair = col;
                val = tmp;
            }
            else if ( (tmp-val) < -0.06 ) {
                Tnode[Tsize] = ipair;
                Tval[Tsize] = val;
                ipair = col;
                val = tmp;
                Tsize ++;
                
            }
            else {
                Tnode[Tsize] = col;
                Tval[Tsize]  = tmp;
                Tsize ++;
            }
        }
        
        if (ipair == -1) {
            map->val[2*nc+1] = -2;
            nc ++;
            i ++;
            continue;
        }
        
        while (Tsize >= 0) {
            Semipd = aggregation_quality_check(A, map1, s1, i, ipair, dopass, k_tg);
            if (!Semipd) {
                ipair = -1;
                l = 0, m = 0, ijtent = 0;
                while (l < Tsize) {
                    if (Tnode[m] >= 0) {
                        tmp = Tval[m];
                        if (ipair == -1) {
                            val   = tmp;
                            ipair = Tnode[m];
                            ijtent= m;
                        }
                        else if ((tmp-val) < -0.06 ) {
                            val = tmp;
                            ipair = Tnode[m];
                            ijtent = m;
                        }
                        l++;
                    }
                    m++;
                }
                Tsize--;
                Tnode[ijtent]=-1;
            }
            else {
                break;
            }
        }
        
        if (ipair == -1) {
            map->val[2*nc+1] = -2;
        }
        else {
            vertices->val[ipair] = nc;
            map->val[2*nc+1] = ipair;
            node ++;
        }
        
        i ++;
        nc ++;
    }
    
    for (i=0; i<row; ++i) s1[i] = s[i];
    
    map->val = (INT*)realloc(map->val, sizeof(INT)*2*nc);
    map->row = 2*nc;
    
    *num_agg = nc;
    
    free(s);
    free(Tnode);
    free(Tval);
}

#endif

static void form_tentative_p (ivector *vertices,
                              dCSRmat *tentp,
                              REAL **basis,
                              INT levelNum,
                              INT num_aggregations)
{
    
    /**
     * \fn static void form_tentative_p (ivector *vertices, dCSRmat *tentp,
     *                                   REAL **basis, INT levelNum, INT num_aggregations)
     *
     * \brief Form aggregation based on strong coupled neighbors
     *
     * \param vertices           Pointer to the aggregation of vertices
     * \param tentp              Pointer to the prolongation operators
     * \param basis              Pointer to the near kernel
     * \param levelNum           Level number
     * \param num_aggregations   Number of aggregations
     *
     * \author Xiaozhe Hu
     * \date   09/29/2009
     *
     * Modified by Xiaozhe Hu on 05/25/2014
     */
    
    INT i, j;
    
    /* Form tentative prolongation */
    tentp->row = vertices->row;
    tentp->col = num_aggregations;
    tentp->nnz = vertices->row;
    
    tentp->IA  = (INT *)calloc(tentp->row+1,sizeof(INT));
    
    // local variables
    INT  *IA = tentp->IA;
    INT  *vval = vertices->val;
    const INT row = tentp->row;
    
    // first run
    for ( i = 0, j = 0; i < row; i++ ) {
        IA[i] = j;
        if (vval[i] > UNPT) j++;
    }
    IA[row] = j;
    
    // allocate memory for P
    tentp->nnz = j;
    tentp->JA  = (INT *)calloc(tentp->nnz, sizeof(INT));
    tentp->val = (REAL *)calloc(tentp->nnz, sizeof(REAL));
    
    INT  *JA = tentp->JA;
    REAL *val = tentp->val;
    
    // second run
    for (i = 0, j = 0; i < row; i ++) {
        IA[i] = j;
        if (vval[i] > UNPT) {
            JA[j] = vval[i];
            val[j] = basis[0][i];
            j ++;
        }
    }
}


static void form_boolean_p (ivector *vertices,
                            dCSRmat *tentp,
                            INT levelNum,
                            INT num_aggregations)
{
    /**
     * \fn static void form_boolean_p (ivector *vertices, dCSRmat *tentp, INT levelNum,
     *                                 INT num_aggregations)
     *
     * \brief Form aggregation based on strong coupled neighbors
     *
     * \param vertices           Pointer to the aggregation of vertices
     * \param tentp              Pointer to the prolongation operators
     * \param levelNum           Level number
     * \param num_aggregations   Number of aggregations
     *
     * \author Xiaozhe Hu
     * \date   09/29/2009
     *
     * Modified by Xiaozhe Hu on 05/25/2014
     */
    
    INT i, j;
    
    /* Form tentative prolongation */
    tentp->row = vertices->row;
    tentp->col = num_aggregations;
    tentp->nnz = vertices->row;
    
    tentp->IA  = (INT *)calloc(tentp->row+1,sizeof(INT));
    
    // local variables
    INT  *IA = tentp->IA;
    INT  *vval = vertices->val;
    const INT row = tentp->row;
    
    // first run
    for ( i = 0, j = 0; i < row; i++ ) {
        IA[i] = j;
        if (vval[i] > UNPT) j++;
    }
    IA[row] = j;
    
    // allocate memory for P
    tentp->nnz = j;
    tentp->JA  = (INT *)calloc(tentp->nnz, sizeof(INT));
    tentp->val = (REAL *)calloc(tentp->nnz, sizeof(REAL));
    
    INT  *JA = tentp->JA;
    REAL *val = tentp->val;
    
    // second run
    for (i = 0, j = 0; i < row; i ++) {
        IA[i] = j;
        if (vval[i] > UNPT) {
            JA[j] = vval[i];
            val[j] = 1.0;
            j ++;
        }
    }
}


static void form_pairwise (const dCSRmat * A,
                           const INT pair,
                           const REAL k_tg,
                           ivector *vertices,
                           INT *num_aggregations)
{
    /**
     * \fn static void form_pairwise (dCSRmat *A,
     *                                INT pair,
     *                                ivector *vertices,
     *                                INT *num_aggregations)
     *
     * \brief Form aggregation based on pairwise matching
     *
     * \param A                 Pointer to the coefficient matrices
     * \param pair              Number of pairs in matching
     * \param vertices          Pointer to the aggregation of vertices
     * \param num_aggregations  Pointer to number of aggregations
     *
     * \author Xiaoping Li, Zheng Li, Chensong Zhang
     * \date   04/21/2014
     *
     * \note Refer to Artem Napov and Yvan Notay "An algebraic multigrid
     *       method with guaranteed convergence rate" 2011.
     */
    
    const INT row  = A->row;
    //const REAL k_tg = 8.0;//7.65;
    
    INT  *AIA  = A->IA;
    INT  *AJA  = A->JA;
    REAL *Aval = A->val;
    
    INT   i, j, row_start, row_end;
    REAL  sum;
    
    INT   col, index = 0;
    REAL  mu, min_mu, aii, ajj, aij;
    REAL  temp1, temp2, temp3, temp4;
    
    /*---------------------------------------------------------*/
    /* Step 1. select extremely strong diagonal dominate rows  */
    /*         and store in G0.                                */
    /*         G0        : vertices->val[i]=G0PT               */
    /*         Remaining : vertices->val[i]=UNPT               */
    /*---------------------------------------------------------*/
    
    ivec_alloc(row, vertices);
    
    if ( pair == 1 ) {
        for ( i = 0; i < row; i++ ) {
            sum = 0.0;
            row_start = AIA[i];
            row_end = AIA[i+1];
            
            for ( j = row_start+1; j < row_end; j++) sum += ABS(Aval[j]);
            
            if ( Aval[AIA[i]] >= ((k_tg+1.)/(k_tg-1.))*sum) {
                vertices->val[i] = G0PT;
            }
            else {
                vertices->val[i] = UNPT;
            }
        }
    }
    else {
        iarray_set(row, vertices->val, UNPT);
    }
    
    /*---------------------------------------------------------*/
    /* Step 2. compute row sum (off-diagonal) for each vertex  */
    /*---------------------------------------------------------*/
    
    REAL *s = (REAL *)calloc(row, sizeof(REAL));
    
    for ( i = 0; i < row; i++ ) {
        s[i] = 0.0;
        
        if ( vertices->val[i] == G0PT ) continue;
        
        row_start = AIA[i]; row_end = AIA[i+1];
        for ( j = row_start + 1; j < row_end; j++ ) s[i] -= Aval[j];
    }
    
    /*---------------------------------------------------------*/
    /* Step 3. start the pairwise aggregation                  */
    /*---------------------------------------------------------*/
    
    *num_aggregations = 0;
    
    for ( i = 0; i < row; i++ ) {
        
        if ( vertices->val[i] != UNPT ) continue;
        
        aij = 0.0;
        min_mu = BIGREAL;
        
        row_start = AIA[i]; row_end = AIA[i+1];
        
        aii = Aval[row_start];
        
        for ( j= row_start + 1; j < row_end; j++ ) {
            col = AJA[j];
            if ( vertices->val[col] != UNPT ) continue;
            
            aij = Aval[j];
            ajj = Aval[AIA[col]];
            
            temp1 = aii+s[i]+2*aij;
            temp2 = ajj+s[col]+2*aij;
            temp2 = 1.0/temp1+1.0/temp2;
            
            temp3 = MAX(ABS(aii-s[i]), SMALLREAL); // avoid temp3 to be zero
            temp4 = MAX(ABS(ajj-s[col]), SMALLREAL); // avoid temp4 to be zero
            temp4 = -aij+1./(1.0/temp3+1.0/temp4);
            
            mu    = (-aij+1.0/temp2) / temp4;
            
            if ( min_mu > mu ) {
                min_mu = mu;
                index  = col;
            }
        }
        
        vertices->val[i] = *num_aggregations;
        
        if ( min_mu <= k_tg ) vertices->val[index] = *num_aggregations;
        
        *num_aggregations += 1;
    }
    
    free(s);
}


static SHORT aggregation_pairwise (AMG_data *mgl,
                                   AMG_param *param,
                                   const INT level,
                                   ivector *vertice,
                                   INT *num_aggregations)
{
    /**
     * \fn static void aggregation_pairwise (dCSRmat *A,
     *                                       AMG_param *param,
     *                                       const INT level,
     *                                       ivector *vertice,
     *                                       INT *num_aggregations)
     *
     * \brief AMG coarsening based on pairwise matching aggregation
     *
     * \param A                 Pointer to dCSRmat
     * \param param             Pointer to AMG parameters
     * \param level             Level number
     * \param vertices          Pointer to the aggregation of vertices
     * \param num_aggregations  Pointer to number of aggregations
     *
     * \author Xiaoping Li, Zheng Li, Chensong Zhang
     * \date   04/21/2014
     *
     * \note Setup A, P, PT and levels using the pairwise aggregation;
     *       Refer to A. Napov and Y. Notay
     *       "An algebraic multigrid method with guaranteed convergence rate", 2012
     *
     * Modified by Xiaozhe Hu on 05/25/2014
     * Modified by Chensong Zhang, Zheng Li on 07/29/2014
     */
    
    const INT  pair_number = param->pair_number;
    dCSRmat  * ptrA = &mgl[level].A;
    REAL       quality_bound = param->quality_bound;
    
    INT        i, j, k, num_agg = 0, aggindex, bndwith;
    INT        lvl = level;
    REAL       isorate;
    
    SHORT      dopass = 0, domin = 0;
    SHORT      status = SUCCESS;
    
#if SYMMETRIC_PAIRWISE == 0
    ivector  map1, map2;
    INT  *order;
    REAL *s = (REAL*)calloc(ptrA->row, sizeof(REAL));
#endif
    
#if SYMMETRIC_PAIRWISE == 1
    dcsr_bandwith(&mgl[level].A, &bndwith);
    if (bndwith > 5.0) 
    param->quality_bound = quality_bound = 1.0*bndwith;
#endif

    for ( i = 1; i <= pair_number; ++i ) {
        
#if SYMMETRIC_PAIRWISE == 1
        /*-- generate aggregations by pairwise matching --*/
        form_pairwise(ptrA, i, quality_bound, &vertice[lvl], &num_agg);
#else
        if ( i == 1 ) {
            first_pairwise_unsymm(ptrA, quality_bound, order, &vertice[lvl], &map1, s, &num_agg);
        }
        else {
            second_pairwise_unsymm(&mgl[level].A, ptrA, quality_bound, i, order, &map1, &vertice[lvl-1], 
			                       &vertice[lvl], &map2, s, &num_agg);
        }
#endif
        
        /*-- check number of aggregates in the first pass --*/
        if ( i == 1 && num_agg < MIN_CDOF ) {
            for ( domin=k=0; k<ptrA->row; k++ ) {
                if ( vertice[lvl].val[k] == G0PT ) domin ++;
            }
            isorate = (REAL)num_agg/domin;
            if ( isorate < 0.1 ) {
                status = ERROR_AMG_COARSEING; goto END;
            }
        }
        
        if ( i < pair_number ) {
            
            /*-- Form Prolongation --*/
            form_boolean_p(&vertice[lvl], &mgl[lvl].P, lvl+1, num_agg);
            
            /*-- Perform aggressive coarsening only up to the specified level --*/
            if ( mgl[lvl].P.col < MIN_CDOF ) break;
            
            /*-- Form restriction --*/
            dcsr_trans(&mgl[lvl].P, &mgl[lvl].R);
            
            /*-- Form coarse level stiffness matrix --*/
            dcsr_rap_agg(&mgl[lvl].R, ptrA, &mgl[lvl].P, &mgl[lvl+1].A);
            
            ptrA = &mgl[lvl+1].A;
            
            dcsr_free(&mgl[lvl].P);
            dcsr_free(&mgl[lvl].R);
        }
        lvl ++; dopass ++;
    }
    
    // Form global aggregation indices
    if ( dopass > 1 ) {
        for ( i = 0; i < mgl[level].A.row; ++i ) {
            aggindex = vertice[level].val[i];
            if ( aggindex < 0 ) continue;
            for ( j = 1; j < dopass; ++j ) aggindex = vertice[level+j].val[aggindex];
            vertice[level].val[i] = aggindex;
        }
    }
    *num_aggregations = num_agg;
        
    /*-- clean memory --*/
    for ( i = 1; i < dopass; ++i ) {
        dcsr_free(&mgl[level+i].A);
        ivec_free(&vertice[level+i]);
    }
    
#if SYMMETRIC_PAIRWISE == 0
    ivec_free(&map1);
    ivec_free(&map2);
    free(s);
#endif
    
END:
    return status;
}


static SHORT aggregation_vmb (dCSRmat *A,
                              ivector *vertices,
                              AMG_param *param,
                              INT levelNum,
                              dCSRmat *Neigh,
                              INT *num_aggregations)
{
    /**
     * \fn static SHORT aggregation_vmb (dCSRmat *A, ivector *vertices, AMG_param *param,
     *                                   INT levelNum, dCSRmat *Neigh, INT *num_aggregations)
     *
     * \brief Form aggregation based on strong coupled neighbors
     *
     * \param A                 Pointer to the coefficient matrices
     * \param vertices          Pointer to the aggregation of vertices
     * \param param             Pointer to AMG parameters
     * \param levelNum          Level number
     * \param Neigh             Pointer to strongly coupled neighbors
     * \param num_aggregations  Pointer to number of aggregations
     *
     * \author Xiaozhe Hu
     * \date   09/29/2009
     *
     * \note Setup A, P, PT and levels using the unsmoothed aggregation algorithm;
     *       Refer to P. Vanek, J. Madel and M. Brezina
     *       "Algebraic Multigrid on Unstructured Meshes", 1994
     *
     */
    
    const INT    row = A->row, col = A->col, nnz = A->IA[row]-A->IA[0];
    const INT  * AIA = A->IA, * AJA = A->JA;
    const REAL * Aval = A->val;
    const INT    max_aggregation = param->max_aggregation;
    
    // return status
    SHORT  status = SUCCESS;
    
    // local variables
    INT    num_left = row;
    INT    subset, count;
    INT  * num_each_agg;
    
    REAL   strongly_coupled, strongly_coupled2;
    INT    i, j, index, row_start, row_end;
    INT  * NIA = NULL, * NJA = NULL;
    REAL * Nval = NULL;
    
    dvector diag;
    dcsr_getdiag(0, A, &diag);  // get the diagonal entries
    
    //if ( GE(param->tentative_smooth, SMALLREAL) ) {
    //    strongly_coupled = param->strong_coupled * pow(0.5, levelNum-1);
    //}
    //else {
    strongly_coupled = param->strong_coupled;
    //}
    strongly_coupled2 = pow(strongly_coupled,2);
    
    /*------------------------------------------*/
    /*    Form strongly coupled neighborhood    */
    /*------------------------------------------*/
    dcsr_alloc(row, col, nnz, Neigh);
    
    NIA  = Neigh->IA;
    NJA  = Neigh->JA;
    Nval = Neigh->val;
    
    for ( i = row; i >= 0; i-- ) NIA[i] = AIA[i];
    
    for ( index = i = 0; i < row; ++i ) {
        NIA[i] = index;
        row_start = AIA[i]; row_end = AIA[i+1];
        for ( j = row_start; j < row_end; ++j ) {
            if ( (AJA[j] == i) 
			  || (pow(Aval[j],2) >= strongly_coupled2*ABS(diag.val[i]*diag.val[AJA[j]]))) {
                NJA[index] = AJA[j];
                Nval[index] = Aval[j];
                index++;
            }
        }
    }
    NIA[row] = index;
    
    Neigh->nnz = index;
    Neigh->JA  = (INT*) realloc(Neigh->JA,  (Neigh->IA[row])*sizeof(INT));
    Neigh->val = (REAL*)realloc(Neigh->val, (Neigh->IA[row])*sizeof(REAL));
    
    NIA  = Neigh->IA;
    NJA  = Neigh->JA;
    Nval = Neigh->val;
    
    dvec_free(&diag);
    
    /*------------------------------------------*/
    /*             Initialization               */
    /*------------------------------------------*/
    ivec_alloc(row, vertices);
    iarray_set(row, vertices->val, -2);
    *num_aggregations = 0;
    
    /*-------------*/
    /*   Step 1.   */
    /*-------------*/
    for ( i = 0; i < row; ++i ) {
        if ( (AIA[i+1] - AIA[i]) == 1 ) {
            vertices->val[i] = UNPT;
            num_left--;
        }
        else {
            subset = TRUE;
            row_start = NIA[i]; row_end = NIA[i+1];
            for ( j = row_start; j < row_end; ++j ) {
                if ( vertices->val[NJA[j]] >= UNPT ) {
                    subset = FALSE;
                    break;
                }
            }
            if ( subset ) {
                count = 0;
                vertices->val[i] = *num_aggregations;
                num_left--;
                count++;
                row_start = NIA[i]; row_end = NIA[i+1];
                for ( j = row_start; j < row_end; ++j ) {
                    if ( (NJA[j]!=i) && (count < max_aggregation) ) {
                        vertices->val[NJA[j]] = *num_aggregations;
                        num_left--;
                        count ++;
                    }
                }
                (*num_aggregations)++;
            }
        }
    }
    
    /*-------------*/
    /*   Step 2.   */
    /*-------------*/
    INT *temp_C = (INT*)calloc(row,sizeof(INT));
    
    if ( *num_aggregations < MIN_CDOF ) {
        status = ERROR_AMG_COARSEING; goto END;
    }
    
    num_each_agg = (INT*)calloc(*num_aggregations,sizeof(INT));
    
    //for ( i = 0; i < *num_aggregations; i++ ) num_each_agg[i] = 0; // initialize
    
    for ( i = row; i--; ) {
        temp_C[i] = vertices->val[i];
        if ( vertices->val[i] >= 0 ) num_each_agg[vertices->val[i]] ++;
    }
    
    for ( i = 0; i < row; ++i ) {
        if ( vertices->val[i] < UNPT ) {
            row_start = NIA[i]; row_end = NIA[i+1];
            for ( j = row_start; j < row_end; ++j ) {
                if ( temp_C[NJA[j]] > UNPT
                    && num_each_agg[temp_C[NJA[j]]] < max_aggregation ) {
                    vertices->val[i] = temp_C[NJA[j]];
                    num_left--;
                    num_each_agg[temp_C[NJA[j]]] ++ ;
                    break;
                }
            }
        }
    }
    
    /*-------------*/
    /*   Step 3.   */
    /*-------------*/
    while ( num_left > 0 ) {
        for ( i = 0; i < row; ++i ) {
            if ( vertices->val[i] < UNPT ) {
                count = 0;
                vertices->val[i] = *num_aggregations;
                num_left--;
                count++;
                row_start = NIA[i]; row_end = NIA[i+1];
                for ( j = row_start; j < row_end; ++j ) {
                    if ( (NJA[j]!=i) && (vertices->val[NJA[j]] < UNPT) 
						             && (count<max_aggregation) ) {
                        vertices->val[NJA[j]] = *num_aggregations;
                        num_left--;
                        count++;
                    }
                }
                (*num_aggregations)++;
            }
        }
    }
    
    free(num_each_agg);
    
END:
    free(temp_C);
    
    return status;
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/
