/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_BCAST_TREE_TAGGED_H_INCLUDED
#define OFI_BCAST_TREE_TAGGED_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"
/*@
 * This function performs a kary/knomial tree based bcast using two-sided communication.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * tree_type - type of tree (kary, knomial_1 or knomial_2)
 * branching_factor - branching factor of the tree
 * This is a blocking bcast function.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Bcast_intra_triggered_tagged(void *buffer, int count,
                                                                    MPI_Datatype datatype, int root,
                                                                    MPIR_Comm * comm_ptr,
                                                                    int tree_type,
                                                                    int branching_factor)
{
    int mpi_errno = MPI_SUCCESS;
    struct fid_cntr *snd_cntr, *rcv_cntr;
    struct fi_deferred_work *works = NULL;
    int ret;
    int myrank, nranks, num_works = 0;
    MPIR_Treealgo_tree_t my_tree;
    int num_children = 0, leaf = 0;

    MPIR_FUNC_ENTER;

    nranks = MPIR_Comm_size(comm_ptr);
    myrank = MPIR_Comm_rank(comm_ptr);

    if (nranks == 1) {
        goto fn_exit;
    }

    /* Invoke the helper function to perform knomial tree-based Ibcast */
    if (tree_type == MPIR_TREE_TYPE_KNOMIAL_1 || tree_type == MPIR_TREE_TYPE_KNOMIAL_2) {
        mpi_errno =
            MPIDI_OFI_Ibcast_knomial_triggered_tagged(buffer, count, datatype, root, comm_ptr,
                                                      tree_type, branching_factor, &num_children,
                                                      &snd_cntr, &rcv_cntr, &works, &my_tree,
                                                      myrank, nranks, &num_works);
    } else {
        /* Invoke the helper function to perform kary tree-based Ibcast */
        mpi_errno =
            MPIDI_OFI_Ibcast_kary_triggered_tagged(buffer, count, datatype, root, comm_ptr,
                                                   branching_factor, &leaf, &num_children,
                                                   &snd_cntr, &rcv_cntr, &works, myrank, nranks,
                                                   &num_works);
    }

    /* Wait for the completion counters to reach their desired values */
    if (!num_children) {
        if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
            do {
                ret = fi_cntr_wait(rcv_cntr, 1, 1);
                MPIDI_OFI_ERR(ret < 0 && ret != -FI_ETIMEDOUT,
                              mpi_errno,
                              MPI_ERR_RMA_RANGE,
                              "**ofid_cntr_wait",
                              "**ofid_cntr_wait %s %d %s %s",
                              __SHORT_FILE__, __LINE__, __func__, fi_strerror(-ret));
                MPID_Progress_test(NULL);
            } while (ret == -FI_ETIMEDOUT);
        } else {
            /* FIXME: This is a workaround for the CXI provider. The performance here will be worse
             * because we're going to be kicking the progress engine much more often than
             * needed. */
            uint64_t donecount = 0;
            while (donecount < 1) {
                donecount = fi_cntr_read(rcv_cntr);
                MPID_Progress_test(NULL);
            }
        }
    } else {
        if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
            do {
                ret = fi_cntr_wait(snd_cntr, num_children, 1);
                MPIDI_OFI_ERR(ret < 0 && ret != -FI_ETIMEDOUT,
                              mpi_errno,
                              MPI_ERR_RMA_RANGE,
                              "**ofid_cntr_wait",
                              "**ofid_cntr_wait %s %d %s %s",
                              __SHORT_FILE__, __LINE__, __func__, fi_strerror(-ret));
                MPID_Progress_test(NULL);
            } while (ret == -FI_ETIMEDOUT);
        } else {
            /* FIXME: This is a workaround for the CXI provider. The performance here will be worse
             * because we're going to be kicking the progress engine much more often than
             * needed. */
            uint64_t donecount = 0;
            while (donecount < num_children) {
                donecount = fi_cntr_read(snd_cntr);
                MPID_Progress_test(NULL);
            }
        }
    }

    if (tree_type == MPIR_TREE_TYPE_KNOMIAL_1 || tree_type == MPIR_TREE_TYPE_KNOMIAL_2) {
        MPIR_Treealgo_tree_free(&my_tree);
    }

    /* Close the counters */
    MPIDI_OFI_CALL(fi_close(&rcv_cntr->fid), cntrclose);
    MPIDI_OFI_CALL(fi_close(&snd_cntr->fid), cntrclose);

    /* Free deferred works */
    MPIDI_OFI_free_deferred_works(works, num_works, false);
    MPL_free(works);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_BCAST_TREE_TAGGED_H_INCLUDED */
