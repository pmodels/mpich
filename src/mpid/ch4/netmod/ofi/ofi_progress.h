/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_PROGRESS_H_INCLUDED
#define OFI_PROGRESS_H_INCLUDED

#include "ofi_impl.h"
#include "ofi_events.h"
#include "ofi_am_events.h"

/* Successful execution of this function could be any of the following
 * - mpi_errno = MPI_SUCCESS while handling cq entries or errors
 * - fi_cq_read returns -FI_EAGAIN
 * Erroneous execution of this function is when mpi_errno != MPI_SUCESS
 * while handling cq entries or errors.
 * Upon successful execution, this function will return the number of
 * completions read during fi_cq_read (0 if -FI_EAGAIN).
 * Upon erroneous execution, this function will return the mpi error
 * codes in the negative integer space to not interfere with the positive
 * values from successful execution.
 * This function will return no negative values that corresponding to OFI
 * error codes. They will be absorbed i.e. 0 will be returned for -FI_EAGAIN
 * and the other fi_cq error codes will be handled.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_test(int vni)
{
    int mpi_errno;
    struct fi_cq_tagged_entry wc[MPIDI_OFI_NUM_CQ_ENTRIES];
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PROGRESS_TEST);

    if (unlikely(MPIDI_OFI_get_buffered(wc, 1))) {
        mpi_errno = MPIDI_OFI_handle_cq_entries(wc, 1);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
        ret = 1;
    } else if (likely(1)) {
        ret = fi_cq_read(MPIDI_OFI_CTX(vni).cq, (void *) wc, MPIDI_OFI_NUM_CQ_ENTRIES);

        if (likely(ret > 0)) {
            mpi_errno = MPIDI_OFI_handle_cq_entries(wc, ret);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        } else if (ret == -FI_EAGAIN)
            ret = 0;
        else {
            mpi_errno = MPIDI_OFI_handle_cq_error(vni, ret);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PROGRESS_TEST);
    return ret;
  fn_fail:
    ret = -mpi_errno;
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vni, int blocking)
{
    int mpi_errno = MPI_SUCCESS, ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_PROGRESS);

    ret = MPIDI_OFI_progress_test(vni);
    if (ret < 0)
        mpi_errno = -ret;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_PROGRESS);

    return mpi_errno;
}

#endif /* OFI_PROGRESS_H_INCLUDED */
