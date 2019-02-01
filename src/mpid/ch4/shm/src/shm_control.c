/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "shm_control.h"

int MPIDI_SHM_ctrl_dispatch(int ctrl_id, void *ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ctrl_id) {
        default:
            /* Unknown SHM control header */
            MPIR_Assert(0);
    }

    return mpi_errno;
}
