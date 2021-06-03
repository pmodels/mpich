/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPID_Psend_init(void *buf, int partitions, MPI_Aint count,
                    MPI_Datatype datatype, int dest, int tag,
                    MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PSEND_INIT);

    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, dest);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_av_is_local(av)) {
        mpi_errno = MPIDI_SHM_mpi_psend_init(buf, partitions, count, datatype, dest, tag,
                                             comm, info, av, request);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_mpi_psend_init(buf, partitions, count, datatype, dest, tag,
                                            comm, info, av, request);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Precv_init(void *buf, int partitions, MPI_Aint count,
                    MPI_Datatype datatype, int source, int tag,
                    MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PRECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PRECV_INIT);

    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, source);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_av_is_local(av)) {
        mpi_errno = MPIDI_SHM_mpi_precv_init(buf, partitions, count, datatype,
                                             source, tag, comm, info, av, request);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_mpi_precv_init(buf, partitions, count, datatype,
                                            source, tag, comm, info, av, request);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PRECV_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
