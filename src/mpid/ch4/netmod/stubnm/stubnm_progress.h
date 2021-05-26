/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBNM_PROGRESS_H_INCLUDED
#define STUBNM_PROGRESS_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBNM_progress(int vci, int blocking)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_PROGRESS_H_INCLUDED */
