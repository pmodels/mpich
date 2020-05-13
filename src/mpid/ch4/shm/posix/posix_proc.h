/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_POSIX_PROC_H_INCLUDED
#define SHM_POSIX_PROC_H_INCLUDED

#include "posix_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rank_is_local(int rank, MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_comm_get_lpid(MPIR_Comm * comm_ptr,
                                                       int idx, int *lpid_ptr, bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SHM_POSIX_PROC_H_INCLUDED */
