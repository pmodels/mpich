/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_PROGRESS_H_INCLUDED
#define SOCK_PROGRESS_H_INCLUDED

#include "sock_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int *made_progress)
{
    /* TODO: not implemented yet */
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SOCK_PROGRESS_H_INCLUDED */
