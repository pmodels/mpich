/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHM_mpi_psend_init(const void *buf, int partitions, MPI_Aint count,
                             MPI_Datatype datatype, int dest, int tag,
                             MPIR_Comm * comm, MPIR_Info * info,
                             MPIDI_av_entry_t * av, MPIR_Request ** request)
{
    return MPIDI_POSIX_mpi_psend_init(buf, partitions, count, datatype, dest, tag, comm,
                                      info, av, request);
}

int MPIDI_SHM_mpi_precv_init(void *buf, int partitions, MPI_Aint count,
                             MPI_Datatype datatype, int source, int tag,
                             MPIR_Comm * comm, MPIR_Info * info,
                             MPIDI_av_entry_t * av, MPIR_Request ** request)
{
    return MPIDI_POSIX_mpi_precv_init(buf, partitions, count, datatype, source, tag, comm,
                                      info, av, request);
}
