/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_PROGRESS_H_INCLUDED
#define SOCK_PROGRESS_H_INCLUDED

#include "sock_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int *made_progress)
{
    if (vci != 0) {
        /* the sock netmod only uses vci 0 */
        return MPI_SUCCESS;
    }

    return MPIDI_SOCK_progress(made_progress);
}

#endif /* SOCK_PROGRESS_H_INCLUDED */
