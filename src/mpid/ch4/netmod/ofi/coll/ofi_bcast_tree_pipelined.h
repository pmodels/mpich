/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_BCAST_TREE_PIPELINED_H_INCLUDED
#define OFI_BCAST_TREE_PIPELINED_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"

/*@
 * This function performs a kary tree based pipelined bcast using one-sided communication.
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * branching_factor - branching factor of the tree
 * chunk_size - chunk size for pipelining
 *
 * This is a blocking bcast function.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Bcast_intra_triggered_pipelined(void *buffer, int count,
                                                                       MPI_Datatype datatype,
                                                                       int root,
                                                                       MPIR_Comm * comm_ptr,
                                                                       int branching_factor,
                                                                       int chunk_size)
{
    int mpi_errno = MPI_SUCCESS;
    struct fid_cntr *snd_cntr, **rcv_cntr = NULL;
    struct fid_mr **r_mr = NULL;
    struct fi_deferred_work *works = NULL;
    int nranks, myrank;
    int ret;
    int i = 0, num_works = 0, leaf = 0, num_children = 0, num_chunks = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_BCAST_INTRA_TRIGGERED_PIPELINED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_BCAST_INTRA_TRIGGERED_PIPELINED);

    nranks = MPIR_Comm_size(comm_ptr);
    myrank = MPIR_Comm_rank(comm_ptr);

    if (nranks == 1) {
        goto fn_exit;
    }

    /* Invoke the helper function to perform one-sided kary tree-based pipelined Ibcast */
    mpi_errno =
        MPIDI_OFI_Ibcast_kary_triggered_pipelined(buffer, count, datatype, root, comm_ptr,
                                                  branching_factor, &leaf, &num_children, &snd_cntr,
                                                  &rcv_cntr, &r_mr, &works, myrank, nranks,
                                                  &num_works, &num_chunks, chunk_size);

    /* Wait for the counters to reach desired values */
    if (!leaf) {
        if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
            do {
                ret = fi_cntr_wait(snd_cntr, num_chunks * num_children, 1);
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
            while (donecount < num_chunks * num_children) {
                donecount = fi_cntr_read(snd_cntr);
                MPID_Progress_test(NULL);
            }
        }
    } else if (leaf) {

        for (i = 0; i < num_chunks; i++) {
            if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
                do {
                    ret = fi_cntr_wait(rcv_cntr[i], 1, 1);
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
                    donecount = fi_cntr_read(rcv_cntr[i]);
                    MPID_Progress_test(NULL);
                }
            }
        }
    }

    /* Free resources */
    for (i = 0; i < num_chunks; i++) {
        MPIDI_OFI_CALL(fi_close(&rcv_cntr[i]->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&r_mr[i]->fid), mr_unreg);
    }
    MPIDI_OFI_CALL(fi_close(&rcv_cntr[i]->fid), cntrclose);

    MPL_free(rcv_cntr);
    MPL_free(r_mr);

    MPIDI_OFI_free_deferred_works(works, num_works, false);
    MPL_free(works);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_BCAST_INTRA_TRIGGERED_PIPELINED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_BCAST_TREE_PIPELINED_H_INCLUDED */
