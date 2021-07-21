/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHM_mpi_psend_init_hook(void *buf, int partitions, MPI_Aint count,
                                  MPI_Datatype datatype, int dest, int tag,
                                  MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    return MPIDI_POSIX_mpi_psend_init_hook(buf, partitions, count, datatype, dest, tag, comm,
                                           info, request);
}

int MPIDI_SHM_mpi_precv_init_hook(void *buf, int partitions, MPI_Aint count,
                                  MPI_Datatype datatype, int source, int tag,
                                  MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    return MPIDI_POSIX_mpi_precv_init_hook(buf, partitions, count, datatype, source, tag, comm,
                                           info, request);
}

int MPIDI_SHM_precv_matched_hook(MPIR_Request * part_req)
{
    return MPIDI_POSIX_precv_matched_hook(part_req);
}
