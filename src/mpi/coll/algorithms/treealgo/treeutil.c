/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"
#include "utarray.h"
#include "treealgo_types.h"
#include "treeutil.h"
#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME tree_add_child
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int tree_add_child(MPII_Treealgo_tree_t * t, int rank)
{
    int mpi_errno = MPI_SUCCESS;

    utarray_push_back(t->children, &rank, MPL_MEM_COLL);
    t->num_children++;

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Treeutil_tree_kary_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Treeutil_tree_kary_init(int rank, int nranks, int k, int root, MPII_Treealgo_tree_t * ct)
{
    int lrank, child;
    int mpi_errno = MPI_SUCCESS;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;

    MPIR_Assert(nranks >= 0);

    if (nranks == 0)
        goto fn_exit;

    lrank = (rank + (nranks - root)) % nranks;

    ct->parent = (lrank == 0) ? -1 : (((lrank - 1) / k) + root) % nranks;

    for (child = 1; child <= k; child++) {
        int val = lrank * k + child;

        if (val >= nranks)
            break;

        val = (val + root) % nranks;
        mpi_errno = tree_add_child(ct, val);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Some examples of knomial_1 tree */
/*     4 ranks                8 ranks
 *       0                      0
 *     /  \                 /   |   \
 *    1   3               1     5    7
 *    |                 /   \   |
 *    2                2     4  6
 *                     |
 *                     3
 */
#undef FUNCNAME
#define FUNCNAME MPII_Treeutil_tree_knomial_1_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Treeutil_tree_knomial_1_init(int rank, int nranks, int k, int root,
                                      MPII_Treealgo_tree_t * ct)
{
    int lrank, i, j, maxtime, tmp, time, parent, current_rank, running_rank, crank;
    int mpi_errno = MPI_SUCCESS;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;

    MPIR_Assert(nranks >= 0);

    if (nranks == 0)
        goto fn_exit;

    lrank = (rank + (nranks - root)) % nranks;
    MPIR_Assert(k >= 2);

    /* maximum number of steps while generating the knomial tree */
    maxtime = 0;
    for (tmp = nranks - 1; tmp; tmp /= k)
        maxtime++;

    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;
    time = 0;
    parent = -1;        /* root has no parent */
    current_rank = 0;   /* start at root of the tree */
    running_rank = current_rank + 1;    /* used for calculation below
                                         * start with first child of the current_rank */

    for (time = 0;; time++) {
        MPIR_Assert(time <= nranks);    /* actually, should not need more steps than log_k(nranks) */

        /* desired rank found */
        if (lrank == current_rank)
            break;

        /* check if rank lies in this range */
        for (j = 1; j < k; j++) {
            if (lrank >= running_rank && lrank < running_rank + MPL_ipow(k, maxtime - time - 1)) {
                /* move to the corresponding subtree */
                parent = current_rank;
                current_rank = running_rank;
                running_rank = current_rank + 1;
                break;
            }

            running_rank += MPL_ipow(k, maxtime - time - 1);
        }
    }

    /* set the parent */
    if (parent == -1)
        ct->parent = -1;
    else
        ct->parent = (parent + root) % nranks;

    /* set the children */
    crank = lrank + 1;  /* crank stands for child rank */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "parent of rank %d is %d, total ranks = %d (root=%d)", rank,
                     ct->parent, nranks, root));
    for (i = time; i < maxtime; i++) {
        for (j = 1; j < k; j++) {
            if (crank < nranks) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "adding child %d to rank %d",
                                 (crank + root) % nranks, rank));
                mpi_errno = tree_add_child(ct, (crank + root) % nranks);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
            crank += MPL_ipow(k, maxtime - i - 1);
        }
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


/* Some examples of knomial_2 tree */
/*     4 ranks               8 ranks
 *       0                      0
 *     /  \                 /   |   \
 *    2    1              4     2    1
 *    |                  / \    |
 *    3                 6   5   3
 *                      |
 *                      7
 */
#undef FUNCNAME
#define FUNCNAME MPII_Treeutil_tree_knomial_2_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Treeutil_tree_knomial_2_init(int rank, int nranks, int k, int root,
                                      MPII_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;
    int lrank, i, j, depth;
    int *flip_bit, child;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;

    MPIR_Assert(nranks >= 0);
    if (nranks <= 0)
        return mpi_errno;

    lrank = (rank + (nranks - root)) % nranks;
    MPIR_Assert(k >= 2);

    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;

    /* Parent calculation */
    if (lrank <= 0)
        ct->parent = -1;
    else {
        depth = MPL_ilog(k, nranks - 1);

        for (i = 0; i < depth; i++) {
            if (MPL_getdigit(k, lrank, i)) {
                ct->parent = (MPL_setdigit(k, lrank, i, 0) + root) % nranks;
                break;
            }
        }
    }

    /* Children calculation */
    depth = MPL_ilog(k, nranks - 1);
    flip_bit = (int *) MPL_calloc(depth, sizeof(int), MPL_MEM_COLL);

    for (j = 0; j < depth; j++) {
        if (MPL_getdigit(k, lrank, j)) {
            break;
        }
        flip_bit[j] = 1;
    }

    for (j = depth - 1; j >= 0; j--) {
        if (flip_bit[j] == 1) {
            for (i = k - 1; i >= 1; i--) {
                child = MPL_setdigit(k, lrank, j, i);
                if (child < nranks)
                    tree_add_child(ct, (child + root) % nranks);
            }
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "parent of rank %d is %d, total ranks = %d (root=%d)", rank,
                     ct->parent, nranks, root));
    MPL_free(flip_bit);

  fn_exit:
    return mpi_errno;
}
