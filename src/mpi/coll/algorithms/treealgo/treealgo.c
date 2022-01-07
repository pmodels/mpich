/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "treealgo.h"
#include "treeutil.h"

int MPII_Treealgo_init(void)
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


int MPIR_Treealgo_tree_create(int rank, int nranks, int tree_type, int k, int root,
                              MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    switch (tree_type) {
        case MPIR_TREE_TYPE_KARY:
            mpi_errno = MPII_Treeutil_tree_kary_init(rank, nranks, k, root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_1:
            mpi_errno = MPII_Treeutil_tree_knomial_1_init(rank, nranks, k, root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_2:
            mpi_errno = MPII_Treeutil_tree_knomial_2_init(rank, nranks, k, root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        default:
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**treetype", "**treetype %d",
                                 tree_type);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


void MPIR_Treealgo_tree_free(MPIR_Treealgo_tree_t * tree)
{
    utarray_free(tree->children);
}
