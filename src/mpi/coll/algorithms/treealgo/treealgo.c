/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "mpiimpl.h"
#include "treealgo.h"
#include "treeutil.h"

int MPII_Treealgo_init()
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_Treealgo_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_Treealgo_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Treealgo_tree_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Treealgo_tree_create(int rank, int nranks, int tree_type, int k, int root,
                              MPII_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_TREEALGO_TREE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_TREEALGO_TREE_INIT);

    switch (tree_type) {
        case MPIR_TREE_TYPE_KARY:
            mpi_errno = MPII_Treeutil_tree_kary_init(rank, nranks, k, root, ct);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_1:
            mpi_errno = MPII_Treeutil_tree_knomial_1_init(rank, nranks, k, root, ct);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_2:
            mpi_errno = MPII_Treeutil_tree_knomial_2_init(rank, nranks, k, root, ct);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;

        default:
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**treetype", "**treetype %d",
                                 tree_type);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_TREEALGO_TREE_INIT);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPII_Treealgo_tree_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPII_Treealgo_tree_free(MPII_Treealgo_tree_t * tree)
{
    utarray_free(tree->children);
}
