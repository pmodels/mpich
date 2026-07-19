/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STUBNM_PROGRESS_H_INCLUDED
#define STUBNM_PROGRESS_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBNM_progress(int vci, int *made_progress)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_PROGRESS_H_INCLUDED */
