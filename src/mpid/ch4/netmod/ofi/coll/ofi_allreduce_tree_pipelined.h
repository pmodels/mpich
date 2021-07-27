/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef OFI_ALLREDUCE_TREE_PIPELINED_H_INCLUDED
#define OFI_ALLREDUCE_TREE_PIPELINED_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_allreduce_util.h"


/*
 * This function performs a kary-tree based pipelined allreduce for large messages using rma.
 * This is a blocking function and the arguments are similar to those of MPI_Allreduce();
 *
 * Input Parameters:
 * sendbuf - send buffer for Allreduce
 * recvbuf - receive buffer for Allreduce
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * op - the operation for Allreduce
 * comm_ptr - communicator
 * branching factor - branching factor of the tree
 * chunk_size - chunk size for pipelining
 */
static inline int MPIDI_OFI_Allreduce_intra_triggered_pipelined(const void *sendbuf, void *recvbuf,
                                                                int count, MPI_Datatype datatype,
                                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                                int branching_factor,
                                                                int chunk_size,
                                                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    struct fid_mr **r_mr = NULL;
    struct fid_cntr **rcv_cntr = NULL, *atomic_cntr, *snd_cntr;
    struct fi_deferred_work *works = NULL;
    int ret, num_children = 0, num_works = 0, leaf = 0, num_chunks = 0, i = 0;

    MPIR_FUNC_ENTER;

    int nranks = MPIR_Comm_size(comm_ptr);
    int myrank = MPIR_Comm_rank(comm_ptr);

    /* Invoke the helper function to perform kary tree-based Iallreduce */
    mpi_errno =
        MPIDI_OFI_Iallreduce_kary_triggered_pipelined(sendbuf, recvbuf, count, datatype, op,
                                                      comm_ptr, branching_factor, &leaf,
                                                      &num_children, &snd_cntr, &rcv_cntr,
                                                      &atomic_cntr, &r_mr, &works, myrank, nranks,
                                                      &num_works, &num_chunks, chunk_size);

    if (nranks > 1) {
        if (num_children) {
            if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
                do {
                    ret = fi_cntr_wait(snd_cntr, num_chunks * num_children, 1);
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
                while (donecount < num_chunks * num_children) {
                    donecount = fi_cntr_read(snd_cntr);
                    MPID_Progress_test(NULL);
                }
            }

        } else {
            for (i = 0; i < num_chunks; i++) {
                if (0 != strcmp(MPIDI_OFI_global.prov_use[0]->fabric_attr->prov_name, "cxi")) {
                    do {
                        ret = fi_cntr_wait(rcv_cntr[i], 2, 1);
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
                        donecount = fi_cntr_read(rcv_cntr[i]);
                        MPID_Progress_test(NULL);
                    }
                }
            }
        }

        /* Close the counters */
        for (i = 0; i < num_chunks; i++) {
            MPIDI_OFI_CALL(fi_close(&rcv_cntr[i]->fid), cntrclose);
            MPIDI_OFI_CALL(fi_close(&r_mr[i]->fid), mr_unreg);
        }

        MPIDI_OFI_CALL(fi_close(&snd_cntr->fid), cntrclose);
        MPIDI_OFI_CALL(fi_close(&atomic_cntr->fid), cntrclose);
        MPL_free(r_mr);
        MPL_free(rcv_cntr);

        /* Free the deferred works */
        MPIDI_OFI_free_deferred_works(works, num_works, false);
        MPL_free(works);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
