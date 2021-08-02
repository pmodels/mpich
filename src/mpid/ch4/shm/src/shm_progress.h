/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_PROGRESS_H_INCLUDED
#define SHM_PROGRESS_H_INCLUDED

#include <shm.h>
#include "../posix/posix_progress.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_progress(int vci, int blocking)
{
    int ret = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_progress(vci, blocking);

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* SHM_PROGRESS_H_INCLUDED */
