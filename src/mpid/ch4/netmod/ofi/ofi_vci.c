/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"

int MPIDI_OFI_comm_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    int mpi_errno = MPI_SUCCESS;
    /* 0. get num_nics from CVARs */
    /* 1. check that MPIDI_OFI_global.n_total_vcis = 0 */
    /* 2. allocate and initialize local vcis */
    /* 3. exchange addresses */
    return mpi_errno;
}
