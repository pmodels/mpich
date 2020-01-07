/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "posix_noinline.h"
#include "release_gather.h"

int MPIDI_POSIX_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;



    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_init_null(comm);
    }


    return mpi_errno;
}

int MPIDI_POSIX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;



    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_free(comm);
    }


    return mpi_errno;
}
