/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPID_Psend_init(const void *buf, int partitions, MPI_Aint count,
                    MPI_Datatype datatype, int dest, int tag,
                    MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, dest);

    CH4_CALL(mpi_psend_init(buf, partitions, count, datatype, dest, tag, comm, info, av, request),
             MPIDI_av_is_local(av), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Precv_init(void *buf, int partitions, MPI_Aint count,
                    MPI_Datatype datatype, int source, int tag,
                    MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, source);

    CH4_CALL(mpi_precv_init(buf, partitions, count, datatype,
                            source, tag, comm, info, av, request),
             MPIDI_av_is_local(av), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
