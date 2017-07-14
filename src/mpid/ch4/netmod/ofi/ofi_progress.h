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

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_progress
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vni, int blocking)
{
    int mpi_errno;
    struct fi_cq_tagged_entry wc[MPIDI_OFI_NUM_CQ_ENTRIES];
    ssize_t ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_PROGRESS);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);

    if (unlikely(MPIDI_OFI_get_buffered(wc, 1)))
        mpi_errno = MPIDI_OFI_handle_cq_entries(wc, 1, 1);
    else if (likely(1)) {
        ret = fi_cq_read(MPIDI_Global.ctx[vni].cq, (void *) wc, MPIDI_OFI_NUM_CQ_ENTRIES);

        if (likely(ret > 0))
            mpi_errno = MPIDI_OFI_handle_cq_entries(wc, ret, 0);
        else if (ret == -FI_EAGAIN)
            mpi_errno = MPI_SUCCESS;
        else
            mpi_errno = MPIDI_OFI_handle_cq_error(vni, ret);
    }

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_PROGRESS);

    return mpi_errno;
}

#endif /* OFI_PROGRESS_H_INCLUDED */
