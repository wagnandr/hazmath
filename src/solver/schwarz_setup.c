/*! \file src/solver/Schwarz_setup.c
 *
 *  Setup phase for the Schwarz methods
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 12/25/15.
 *  Copyright 2015__HAZMATH__. All rights reserved.
 *
 *  \note  Done cleanup for releasing -- Xiaozhe Hu 03/12/2017
 *
*/

#include "hazmath.h"

static void Schwarz_levels (INT, dCSRmat *, INT *, INT *, INT *, INT *, INT);

/*---------------------------------*/
/*--      Public Functions       --*/
/*---------------------------------*/

/**
 * \fn void Schwarz_get_block_matrix (Schwarz_data *Schwarz, INT nblk,
 *                                    INT *iblock, INT *jblock, INT *mask)
 *
 * \brief Form Schwarz partition data
 *
 * \param Schwarz Pointer to the Schwarz data
 * \param nblk    Number of partitions
 * \param iblock  Pointer to number of vertices on each level
 * \param jblock  Pointer to vertices of each level
 * \param mask    Pointer to flag array
 *
 * \note  This needs to be rewritten -- Xiaozhe
 */
void Schwarz_get_block_matrix (Schwarz_data *Schwarz,
                                    INT nblk,
                                    INT *iblock,
                                    INT *jblock,
                                    INT *mask)
{
    INT i, j, iblk, ki, kj, kij, is, ibl0, ibl1, nloc, iaa, iab;
    INT maxbs = 0, count, nnz;

    dCSRmat A = Schwarz->A;
    dCSRmat *blk = Schwarz->blk_data;

    INT  *ia  = A.IA;
    INT  *ja  = A.JA;
    REAL *val = A.val;

    // get maximal block size
    for (is=0; is<nblk; ++is) {
        ibl0 = iblock[is];
        ibl1 = iblock[is+1];
        nloc = ibl1-ibl0;
        maxbs = MAX(maxbs, nloc);
    }

    Schwarz->maxbs = maxbs;

    // allocate memory for each sub_block's right hand
    Schwarz->xloc1   = dvec_create(maxbs);
    Schwarz->rhsloc1 = dvec_create(maxbs);

    for (is=0; is<nblk; ++is) {
        ibl0 = iblock[is];
        ibl1 = iblock[is+1];
        nloc = ibl1-ibl0;
        count = 0;
        for (i=0; i<nloc; ++i ) {
            iblk = ibl0 + i;
            ki   = jblock[iblk];
            iaa  = ia[ki];
            iab  = ia[ki+1];
            count += iab - iaa;
            mask[ki] = i+1;  // The +1 -Peter
        }

        blk[is] = dcsr_create(nloc, nloc, count);
        blk[is].IA[0] = 0;
        nnz = 0;

        for (i=0; i<nloc; ++i) {
            iblk = ibl0 + i;
            ki = jblock[iblk];
            iaa = ia[ki];
            iab = ia[ki+1];
            for (kij = iaa; kij<iab; ++kij) {
                kj = ja[kij];
                j  = mask[kj];
                if(j != 0) {
                    blk[is].JA[nnz] = j-1; // The -1 corresponds with +1 above. -Peter
                    blk[is].val[nnz] = val[kij];
                    nnz ++;
                }
            }
            blk[is].IA[i+1] = nnz;
        }

        blk[is].nnz = nnz;

        // zero the mask so that everyting is as it was
        for (i=0; i<nloc; ++i) {
            iblk = ibl0 + i;
            ki   = jblock[iblk];
            mask[ki] = 0;
        }
    }
}

/**
 * \fn INT Schwarz_setup (Schwarz_data *Schwarz, Schwarz_param *param)
 *
 * \brief Setup phase for the Schwarz methods
 *
 * \param Schwarz    Pointer to the Schwarz data
 * \param param      Type of the Schwarz method
 *
 * \return           SUCCESS if succeed
 *
 */
INT Schwarz_setup (Schwarz_data *Schwarz,
                   Schwarz_param *param)
{
    // information about A
    dCSRmat A = Schwarz->A;
    INT n   = A.row;

    INT  block_solver = param->Schwarz_blksolver;
    INT  maxlev = param->Schwarz_maxlvl;
    Schwarz->swzparam = param;

    // local variables
    INT i;
    INT inroot = -10, nsizei = -10, nsizeall = -10, nlvl = 0;
    INT *jb=NULL;
    ivector MaxIndSet;

    // data for Schwarz method
    INT nblk;
    INT *iblock = NULL, *jblock = NULL, *mask = NULL, *maxa = NULL;

    // return
    INT flag = SUCCESS;

    // allocate memory
    maxa    = (INT *)calloc(n,sizeof(INT));
    mask    = (INT *)calloc(n,sizeof(INT));
    iblock  = (INT *)calloc(n,sizeof(INT));
    jblock  = (INT *)calloc(n,sizeof(INT));

    nsizeall=0;
    memset(mask,   0, sizeof(INT)*n);
    memset(iblock, 0, sizeof(INT)*n);
    memset(maxa,   0, sizeof(INT)*n);

    maxa[0]=0;

    // select root nodes.
    MaxIndSet = sparse_MIS(&A);

    /*-------------------------------------------*/
    // find the blocks
    /*-------------------------------------------*/
    // first pass: do a maxlev level sets out for each node
    for ( i = 0; i < MaxIndSet.row; i++ ) {
        inroot = MaxIndSet.val[i];
        Schwarz_levels(inroot,&A,mask,&nlvl,maxa,jblock,maxlev);
        nsizei=maxa[nlvl];
        nsizeall+=nsizei;
    }

    /* We only calculated the size of this up to here. So we can reallocate jblock */
    jblock = (INT *)realloc(jblock,(nsizeall+n)*sizeof(INT));

    // second pass: redo the same again, but this time we store in jblock
    maxa[0]=0;
    iblock[0]=0;
    nsizeall=0;
    jb=jblock;
    for (i=0;i<MaxIndSet.row;i++) {
        inroot = MaxIndSet.val[i];
        Schwarz_levels(inroot,&A,mask,&nlvl,maxa,jb,maxlev);
        nsizei=maxa[nlvl];
        iblock[i+1]=iblock[i]+nsizei;
        nsizeall+=nsizei;
        jb+=nsizei;
    }
    nblk = MaxIndSet.row;

    /*-------------------------------------------*/
    //  LU decomposition of blocks
    /*-------------------------------------------*/
    memset(mask, 0, sizeof(INT)*n);
    Schwarz->blk_data = (dCSRmat*)calloc(nblk, sizeof(dCSRmat));
    Schwarz_get_block_matrix(Schwarz, nblk, iblock, jblock, mask);

    // Setup for each block solver
    switch (block_solver) {

#if WITH_SUITESPARSE
        case SOLVER_UMFPACK: {
            /* use UMFPACK direct solver on each block */
            dCSRmat *blk = Schwarz->blk_data;
            void **numeric	= (void**)calloc(nblk, sizeof(void*));
            dCSRmat Ac_tran;
            //printf("number of blocks = %d\n",nblk);
            for (i=0; i<nblk; ++i) {
                Ac_tran = dcsr_create(blk[i].row, blk[i].col, blk[i].nnz);
                dcsr_transz(&blk[i], NULL, &Ac_tran);
                dcsr_cp(&Ac_tran, &blk[i]);
                //printf("size of block %d: nrow=%d, nnz=%d\n",i, blk[i].row, blk[i].nnz);
                numeric[i] = umfpack_factorize(&blk[i], 0);
            }
            Schwarz->numeric = numeric;
            dcsr_free(&Ac_tran);

            break;
        }
#endif

        default: {
            /* do nothing for iterative methods */
        }
    }

    /*-------------------------------------------*/
    //  return
    /*-------------------------------------------*/
    Schwarz->nblk   = nblk;
    Schwarz->iblock = iblock;
    Schwarz->jblock = jblock;
    Schwarz->mask   = mask;
    Schwarz->maxa   = maxa;
    Schwarz->Schwarz_type = param->Schwarz_type;
    Schwarz->blk_solver = param->Schwarz_blksolver;

    printf("Schwarz method setup is done! Find %d blocks\n",nblk);

    return flag;
}

/**
 * \fn void Schwarz_get_patch_geometric (Schwarz_data *Schwarz,
 *                                       mesh_struct *mesh,
 *                                       INT patchType)
 *
 * \brief Form Schwarz partition data
 *
 * \param Schwarz Pointer to the Schwarz data
 *
 */
void Schwarz_get_patch_geometric (Schwarz_data *Schwarz,
                                  mesh_struct *mesh,
                                  INT patchTypeIN,
                                  INT patchTypeOUT)
{
    INT nblk, ntot;

    INT* iblk;
    INT* jblk;

    // patchType to element
    iCSRmat p_el;
    iCSRmat p_p;

    switch ( patchTypeIN ) {
      case 0: // vertex
        nblk = mesh->nv;
        icsr_trans(mesh->el_v, &p_el);
        break;
      case 1: // edge
        nblk = mesh->nedge;
        icsr_trans(mesh->el_ed,&p_el);
        break;
      case 2: // face
        nblk = mesh->nface;
        icsr_trans(mesh->el_f, &p_el);
        break;
      default:
        // Throw error
        break;
    }

    switch ( patchTypeOUT ) {
      case 0: // vertex
        icsr_mxm_symb( &p_el, mesh->el_v, &p_p);
        ntot = p_p.nnz;
        break;
      case 1: // edge
        icsr_mxm_symb( &p_el, mesh->el_ed, &p_p);
        ntot = p_p.nnz;
        break;
      case 2: // face
        icsr_mxm_symb( &p_el, mesh->el_f, &p_p);
        ntot = p_p.nnz;
        break;
      default:
        // Throw error
        break;
    }

    iblk = (INT*)calloc(nblk+1,sizeof(INT));
    iarray_cp(nblk+1,p_p.IA,iblk);

    jblk = (INT*)calloc(ntot,sizeof(INT));
    iarray_cp(ntot,p_p.JA,jblk);

    Schwarz->nblk = nblk;
    Schwarz->iblock = iblk;
    Schwarz->jblock = jblk;

    return;
}

/**
 * \fn void Schwarz_get_patch_geometric_multiple_DOFtype (Schwarz_data *Schwarz,
 *                                                        mesh_struct *mesh,
 *                                                        INT patchTypeIN,
 *                                                        INT* patchTypeOUT,
 *                                                        INT nptype)
 *
 * \brief Form Schwarz partition data
 *
 * \param Schwarz       Pointer to the Schwarz data
 * \param mesh
 * \param patchTypeIN   Generating DOF type (0 vertex, 1 edge, 2 face) for patch
 * \param patchTypeOUT  Pointer to array of DOF types in patch. nptype entries.
 * \param nptype        Number of DOF types in patch.
 *
 * \note example:
 *       For use in something like bubble-enriched P1 in 2D.
 *       patchTypeOUT = {1,0,0}; nptype = 3;
 *       Results in a patch that is edges, vertices, vertices. Each vertex will then appear twice.
 *       The DOFs are indexed up by the number of DOFs in the previous patch type.
 *
 */
void Schwarz_get_patch_geometric_multiple_DOFtype (Schwarz_data *Schwarz,
                                                   mesh_struct *mesh,
                                                   INT patchTypeIN,
                                                   INT* patchTypeOUT,
                                                   INT nptype)
{
    INT nblk, ntot;
    INT i,blk;
    //INT* ntotpatch = (INT*)calloc(nptype,sizeof(INT));
    INT* dofshift  = (INT*)calloc(nptype+1,sizeof(INT));
    dofshift[0] = 0;

    INT* iblk;
    INT* jblk;

    // patchType to element
    iCSRmat p_el;
    iCSRmat temp;
    //iCSRmat p_p;
    iCSRmat *p_p = (iCSRmat*)calloc(nptype,sizeof(iCSRmat));

    switch ( patchTypeIN ) {
      case 0: // elm
        nblk = mesh->nelm;
        p_el = icsr_create_identity(nblk,0);
        break;
      case 1: // vertex
        nblk = mesh->nv;
        icsr_trans(mesh->el_v, &p_el);
        break;
      case 2: // edge
        nblk = mesh->nedge;
        icsr_trans(mesh->el_ed,&p_el);
        break;
      case 3: // face
        nblk = mesh->nface;
        icsr_trans(mesh->el_f, &p_el);
        break;
      case 4: // elm_face_elm
        nblk = mesh->nelm;
        icsr_trans(mesh->el_f, &temp);
        icsr_mxm_symb( mesh->el_f, &temp, &p_el);
        break;
      default:
        // Throw error
        break;
    }

    ntot = 0;
    for( i=0; i<nptype; i++){
      printf("working through %d, patch type %d\n",i,patchTypeOUT[i]);
      switch ( patchTypeOUT[i] ) {
        case 0: // elm
          p_p[i] = icsr_create( p_el.row, p_el.col, p_el.nnz);//TODO: Replace with smarter allocation
          iarray_cp (p_p[i].row+1, p_el.IA, p_p[i].IA);
          iarray_cp (p_p[i].nnz, p_el.JA, p_p[i].JA);
          ntot += p_p[i].nnz;
          dofshift[i+1] = mesh->nelm + dofshift[i];
          break;
        case 1: // vertex
          icsr_mxm_symb( &p_el, mesh->el_v, p_p+i);
          ntot += p_p[i].nnz;
          dofshift[i+1] = mesh->nv + dofshift[i];
          break;
        case 2: // edge
          if( patchTypeIN == 1){
            icsr_trans(mesh->ed_v, p_p+i);
            ntot += p_p[i].nnz;
            dofshift[i+1] = mesh->nedge + dofshift[i];
          } else {
            icsr_mxm_symb( &p_el, mesh->el_ed, p_p+i);
            ntot += p_p[i].nnz;
            dofshift[i+1] = mesh->nedge + dofshift[i];
          }
          break;
        case 3: // face
          if( patchTypeIN == 1){//TODO: REMOVE THIS EDIT, change back to == 1
            icsr_trans(mesh->f_v, p_p+i);
            ntot += p_p[i].nnz;
            dofshift[i+1] = mesh->nface + dofshift[i];
          } else {
            icsr_mxm_symb( &p_el, mesh->el_f, p_p+i);
            ntot += p_p[i].nnz;
            dofshift[i+1] = mesh->nface + dofshift[i];
          }
          break;
        case 11: // Single Vertex //TODO: This only works if seed is vertex
          *(p_p+i) = icsr_create_identity( mesh->nv, 0 );
          ntot += p_p[i].nnz;
          dofshift[i+1] = mesh->nv + dofshift[i];
          break;
        default:
          // Throw error
          break;
      }
    }

    iblk = (INT*)calloc(nblk+1,sizeof(INT));
    for( blk=0; blk<nblk+1; blk++){
      for( i=0; i<nptype; i++){
        iblk[blk] += p_p[i].IA[blk];
      }
    }
//    iarray_cp(nblk+1,p_p.IA,iblk);
//
    INT ia, ib;
    INT j;
    INT cnt = 0;
    jblk = (INT*)calloc(ntot,sizeof(INT));
    for( blk=0; blk<nblk; blk++){
      for( i=0; i<nptype; i++){

        ia = p_p[i].IA[blk];
        ib = p_p[i].IA[blk+1];

        for( j=ia; j<ib; j++ ){
          jblk[ cnt ] = p_p[i].JA[ j ] + dofshift[i];
          cnt++;
        }

      }
    }
//    iarray_cp(ntot,p_p.JA,jblk);
//
    Schwarz->nblk = nblk;
    Schwarz->iblock = iblk;
    Schwarz->jblock = jblk;

    return;
}

/**
 * \fn INT Schwarz_setup_geometric (Schwarz_data *Schwarz, Schwarz_param *param)
 *
 * \brief Setup phase for the Schwarz methods
 *
 * \param Schwarz    Pointer to the Schwarz data
 * \param param      Type of the Schwarz method
 * \param mesh       Pointer to mesh
 *
 * \return           SUCCESS if succeed
 *
 */
INT Schwarz_setup_geometric (Schwarz_data *Schwarz,
                             Schwarz_param *param,
                             mesh_struct *mesh)
{
    // information about A
    dCSRmat A = Schwarz->A;
    INT n   = A.row;

    INT  block_solver = param->Schwarz_blksolver;
    Schwarz->swzparam = param;

    // local variables
    //INT i;

    // data for Schwarz method
    INT nblk;
    //INT *iblock = NULL, *jblock = NULL;
    INT *mask = NULL, *maxa = NULL;

    // return
    INT flag = SUCCESS;

    // allocate memory
    maxa    = (INT *)calloc(n,sizeof(INT));
    mask    = (INT *)calloc(n,sizeof(INT));

    memset(mask,   0, sizeof(INT)*n);
    memset(maxa,   0, sizeof(INT)*n);

    maxa[0]=0;

    /*-------------------------------------------*/
    // find the blocks
    /*-------------------------------------------*/
    printf("Finding Schwarz patches\n");
    INT* patch_type_out = param->patch_type_gmg+2;
    INT patch_type_in = param->patch_type_gmg[1];
    INT n_patch_out = param->patch_type_gmg[0];
    Schwarz_get_patch_geometric_multiple_DOFtype( Schwarz, mesh, patch_type_in, patch_type_out, n_patch_out);
    printf("Found Schwarz patches\n");
    nblk = Schwarz->nblk;

    /*-------------------------------------------*/
    //  LU decomposition of blocks
    /*-------------------------------------------*/
    memset(mask, 0, sizeof(INT)*n);
    Schwarz->blk_data = (dCSRmat*)calloc(nblk, sizeof(dCSRmat));
    printf("Getting block matrix\n");
    Schwarz_get_block_matrix(Schwarz, nblk, Schwarz->iblock, Schwarz->jblock, mask);
    printf("Got block matrix\n");

    // Setup for each block solver
    switch (block_solver) {

#if WITH_SUITESPARSE
        case SOLVER_UMFPACK: {
            /* use UMFPACK direct solver on each block */
            dCSRmat *blk = Schwarz->blk_data;
            void **numeric	= (void**)calloc(nblk, sizeof(void*));
            dCSRmat Ac_tran;
            //printf("number of blocks = %d\n",nblk);
            INT i;
            for (i=0; i<nblk; ++i) {
                Ac_tran = dcsr_create(blk[i].row, blk[i].col, blk[i].nnz);
                dcsr_transz(&blk[i], NULL, &Ac_tran);
  //printf("BLK: %d\n",i);
  //csr_print_matlab(stdout,&blk[i]);
                dcsr_cp(&Ac_tran, &blk[i]);
                //printf("size of block %d: nrow=%d, nnz=%d\n",i, blk[i].row, blk[i].nnz);
                numeric[i] = umfpack_factorize(&blk[i], 0);
            }
            Schwarz->numeric = numeric;
            dcsr_free(&Ac_tran);

            break;
        }
#endif

        default: {
            /* do nothing for iterative methods */
        }
    }

    /*-------------------------------------------*/
    //  return
    /*-------------------------------------------*/
    Schwarz->mask   = mask;
    Schwarz->maxa   = maxa;
    Schwarz->Schwarz_type = param->Schwarz_type;
    Schwarz->blk_solver = param->Schwarz_blksolver;

    printf("Schwarz method setup is done! Find %d blocks\n",nblk);

    return flag;
}

/*---------------------------------*/
/*--      Private Functions      --*/
/*---------------------------------*/

/**
 * \fn static void Schwarz_levels (INT inroot, dCSRmat *A, INT *mask, INT *nlvl,
 *                                 INT *iblock, INT *jblock, INT maxlev)
 *
 * \brief Form the level hierarchy of input root node
 *
 * \param inroot  Root node
 * \param A       Pointer to CSR matrix
 * \param mask    Pointer to flag array
 * \param nlvl    The number of levels to expand from root node
 * \param iblock  Pointer to vertices number of each level
 * \param jblock  Pointer to vertices of each level
 * \param maxlev  The maximal number of levels to expand from root node
 *
 * \note  This needs to be rewritten -- Xiaozhe
 *
 */
static void Schwarz_levels (INT inroot,
                            dCSRmat *A,
                            INT *mask,
                            INT *nlvl,
                            INT *iblock,
                            INT *jblock,
                            INT maxlev)
{
    INT *ia = A->IA;
    INT *ja = A->JA;
    INT nnz = A->nnz;
    INT i, j, lvl, lbegin, lvlend, nsize, node;
    INT jstrt, jstop, nbr, lvsize;

    // This is diagonal
    if (ia[inroot+1]-ia[inroot] <= 1) {
        lvl = 0;
        iblock[lvl] = 0;
        jblock[iblock[lvl]] = inroot;
        lvl ++;
        iblock[lvl] = 1;
    }
    else {
        // input node as root node (level 0)
        lvl = 0;
        jblock[0] = inroot;
        lvlend = 0;
        nsize  = 1;
        // mark root node
        mask[inroot] = 1; //??

        lvsize = nnz;

        // start to form the level hierarchy for root node(level1, level2, ... maxlev)
        while (lvsize > 0 && lvl < maxlev) {
            lbegin = lvlend;
            lvlend = nsize;
            iblock[lvl] = lbegin;
            lvl ++;
            for(i=lbegin; i<lvlend; ++i) {
                node = jblock[i];
                jstrt = ia[node];
                jstop = ia[node+1];
                for (j = jstrt; j<jstop; ++j) {
                    nbr = ja[j];
                    if (mask[nbr] == 0) {
                        jblock[nsize] = nbr;
                        mask[nbr] = lvl;
                        nsize ++;
                    }
                }
            }
            lvsize = nsize - lvlend;
        }

        iblock[lvl] = nsize;

        // reset mask array
        for (i = 0; i< nsize; ++i) {
            node = jblock[i];
            mask[node] = 0;
        }
    }

    *nlvl = lvl;
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/
