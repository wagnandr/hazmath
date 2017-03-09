/*! \file src/utilities/data.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 12/23/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 *  \note modified by Xiaozhe Hu 10/27/2016
 *  \note: done cleanup for releasing -- Xiaozhe Hu 10/27/2016
 *
 */

#include "hazmath.h"

/***********************************************************************************************/
void precond_data_null (precond_data *pcdata)
{
    /*!
     * \fn void precond_data_null (precond_data *pcdata)
     *
     * \brief Initialize precond_data (pointers are set to NULL)
     *
     * \param pcdata   Preconditioning data structure
     *
     */
    
    pcdata->AMG_type            = UA_AMG;
    pcdata->print_level         = PRINT_MIN;
    pcdata->maxit               = 100;
    pcdata->max_levels          = 20;
    pcdata->tol                 = 1e-8;
    pcdata->cycle_type          = V_CYCLE;
    pcdata->smoother            = SMOOTHER_GS;
    pcdata->presmooth_iter      = 1;
    pcdata->postsmooth_iter     = 1;
    pcdata->relaxation          = 1.2;
    pcdata->polynomial_degree   = 2;
    pcdata->coarsening_type     = 1;
    pcdata->coarse_solver       = SOLVER_UMFPACK;
    pcdata->coarse_scaling      = OFF;
    pcdata->amli_degree         = 2;
    pcdata->nl_amli_krylov_type = SOLVER_VFGMRES;

    pcdata->amli_coef           = NULL;
    pcdata->mgl_data            = NULL;
    pcdata->A                   = NULL;

    pcdata->A_nk                = NULL;
    pcdata->P_nk                = NULL;
    pcdata->R_nk                = NULL;

    pcdata->r                   = NULL;
    pcdata->w                   = NULL;

}


/***********************************************************************************************/
AMG_data *amg_data_create(SHORT max_levels)
{
    /*!
     * \fn AMG_data * amg_data_create (SHORT max_levels)
     *
     * \brief Create AMG_data structure (but all values are 0 and pointers point to NULL)
     *
     * \param max_levels   Max number of levels allowed
     *
     * \return Pointer to the AMG_data structure
     *
     */
    
    max_levels = MAX(1, max_levels); // at least allocate one level
    
    AMG_data *mgl = (AMG_data *)calloc(max_levels,sizeof(AMG_data));
    
    INT i;
    for ( i=0; i<max_levels; ++i ) {
        mgl[i].max_levels = max_levels;
        mgl[i].num_levels = 0;
        mgl[i].near_kernel_dim = 0;
        mgl[i].near_kernel_basis = NULL;
        mgl[i].cycle_type = 0;
    }
    
    return(mgl);
}


/***********************************************************************************************/
void amg_data_free(AMG_data *mgl,
                   AMG_param *param)
{
    /*!
     * \fn void amg_data_free(AMG_data *mgl, AMG_param *param)
     *
     * \brief Free AMG_data structure
     *
     * \param mgl    Pointer to the AMG_data
     * \param param  Pointer to AMG parameters
     *
     *
     */
    
    const INT max_levels = MAX(1,mgl[0].num_levels);
    
    INT i;
    
    for (i=0; i<max_levels; ++i) {
        dcsr_free(&mgl[i].A);
        dcsr_free(&mgl[i].P);
        dcsr_free(&mgl[i].R);
        dvec_free(&mgl[i].b);
        dvec_free(&mgl[i].x);
        dvec_free(&mgl[i].w);
        ivec_free(&mgl[i].cfmark);
    }

    for (i=0; i<mgl->near_kernel_dim; ++i) {
        if (mgl->near_kernel_basis[i]) free(mgl->near_kernel_basis[i]);
        mgl->near_kernel_basis[i] = NULL;
    }
    
    // Clean direct solver data if necessary
    switch (param->coarse_solver) {
            
#if WITH_SUITESPARSE
        case SOLVER_UMFPACK: {
            umfpack_free_numeric(mgl[max_levels-1].Numeric);
            break;
        }
#endif
            
        default: // Do nothing!
            break;
    }
    
    free(mgl->near_kernel_basis);
    mgl->near_kernel_basis = NULL;
    
    free(mgl); mgl = NULL;
    
    if (param != NULL) {
        if ( param->cycle_type == AMLI_CYCLE )
            free(param->amli_coef);
    }
}

/***********************************************************************************************/
void HX_curl_data_null (HX_curl_data *hxcurldata)
{
    /*!
     * \fn void HX_curl_data_null(HX_curl_data *hxcurldata)
     *
     * \brief Initalize HX_curl_data structure (set values to 0 and pointers to NULL)
     *
     * \param hxcurldata    Pointer to the HX_curl_data structure
     *
     */

    hxcurldata->A               = NULL;

    hxcurldata->smooth_type     = 0;
    hxcurldata->smooth_iter     = 0;

    hxcurldata->P_curl          = NULL;
    hxcurldata->Pt_curl         = NULL;
    hxcurldata->A_vgrad         = NULL;

    hxcurldata->amgparam_vgrad  = NULL;
    hxcurldata->mgl_vgrad       = NULL;

    hxcurldata->Grad            = NULL;
    hxcurldata->Gradt           = NULL;
    hxcurldata->A_grad          = NULL;

    hxcurldata->amgparam_grad   = NULL;
    hxcurldata->mgl_grad        = NULL;

    hxcurldata->backup_r        = NULL;
    hxcurldata->w               = NULL;

}

/***********************************************************************************************/
void HX_curl_data_free (HX_curl_data *hxcurldata, SHORT flag)
{
    /*!
     * \fn void HX_curl_data_free (HX_curl_data *hxcurldata, SHORT flag)
     *
     * \brief Free HX_curl_data structure (set values to 0 and pointers to NULL)
     *
     * \param hxcurldata    Pointer to the HX_curl_data structure
     * \param flag          flag of whether the date will be reused:
     *                          flag = False - A, P_curl, and Grad will be reused
     *                          flag = TRUE  - free everything
     *
     */
    
    if (flag == TRUE) {
        dcsr_free(hxcurldata->A);
        dcsr_free(hxcurldata->P_curl);
        dcsr_free(hxcurldata->Grad);
    }
    
    dcsr_free(hxcurldata->Pt_curl);
    dcsr_free(hxcurldata->A_vgrad);
    
    amg_data_free(hxcurldata->mgl_vgrad, hxcurldata->amgparam_vgrad);
    
    dcsr_free(hxcurldata->Gradt);
    dcsr_free(hxcurldata->A_grad);
    
    amg_data_free(hxcurldata->mgl_grad, hxcurldata->amgparam_grad);
    
    if (hxcurldata->backup_r) free(hxcurldata->backup_r);
    
    if (hxcurldata->w) free(hxcurldata->w);
        

}

/***********************************************************************************************/
void precond_null(precond *pcdata)
{
    /**
     * \fn void precond_null(precond *pcdata)
     *
     * \brief Initialize precond data (set pointers to NULL)
     *
     * \param pcdata   Pointer to precond
     *
     */
    
    pcdata->data = NULL;
    pcdata->fct  = NULL;
}

/*************************************  END  ***************************************************/
