/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_ALLREDUCE_TREE_TAGGED_H_INCLUDED
#define OFI_ALLREDUCE_TREE_TAGGED_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_allreduce_util.h"

/*
 * This function performs a tree based allreduce using two-sided communication.
 * This is a blocking function and the arguments are same a in MPI_Allreduce();
 *
 * Input Parameters:
 * sendbuf - send buffer for Allreduce
 * recvbuf - receive buffer for Allreduce
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * op - the operation for Allreduce
 * comm_ptr - communicator
 * tree_type - type of tree (knomial_1, knomial_2, or kary)
 * branching_factor - branching factor of the tree
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Allreduce_intra_triggered_tagged(const void *sendbuf,
                                                                        void *recvbuf, int count,
                                                                        MPI_Datatype datatype,
                                                                        MPI_Op op,
                                                                        MPIR_Comm * comm_ptr,
                                                                        int tree_type,
                                                                        int branching_factor,
                                                                        MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    struct fid_cntr *snd_cntr, *rcv_cntr, *atomic_cntr;
    struct fi_deferred_work *works = NULL;
    int ret;
    int myrank, nranks, num_works = 0;
    MPIR_Treealgo_tree_t my_tree;
    int num_children = 0;

    MPIR_FUNC_ENTER;

    nranks = MPIR_Comm_size(comm_ptr);
    myrank = MPIR_Comm_rank(comm_ptr);

    /* Invoke the helper function to perform tree-based Iallreduce */
    mpi_errno =
        MPIDI_OFI_Iallreduce_tree_triggered_tagged(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   tree_type, branching_factor, &num_children,
                                                   &snd_cntr, &rcv_cntr, &atomic_cntr, &works,
                                                   &my_tree, myrank, nranks, &num_works);

    if (nranks > 1) {

        /* Wait till the coompletion counters reach their desired values */
        if (num_children) {
            if (MPIDI_OFI_global.using_cxi) {
                do {
                    ret = fi_cntr_wait(snd_cntr, num_children, 1);
                    MPIDI_OFI_ERR(ret < 0 &&
                                  ret != -FI_ETIMEDOUT, mpi_errno, MPI_ERR_RMA_RANGE,
                                  "**ofid_cntr_wait", "**ofid_cntr_wait %s %d %s %s",
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
        } else {
            if (MPIDI_OFI_global.using_cxi) {
                do {
                    ret = fi_cntr_wait(rcv_cntr, 2, 1);
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
                while (donecount < 2) {
                    donecount = fi_cntr_read(rcv_cntr);
                    MPID_Progress_test(NULL);
                }
            }
        }

        /* Close the counters */
        MPIDI_OFI_CALL(fi_close(&rcv_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&snd_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&atomic_cntr->fid), cntrclose);

        /* Free the deferred works */
        MPIDI_OFI_free_deferred_works(works, num_works, false);
        MPL_free(works);

        if (tree_type == MPIR_TREE_TYPE_KNOMIAL_1 || tree_type == MPIR_TREE_TYPE_KNOMIAL_2) {
            MPIR_Treealgo_tree_free(&my_tree);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif
