/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_PART_H_INCLUDED
#define CH4_PART_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_part_start(MPIR_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_part_start(request);
#else
    if (MPIDI_REQUEST(request, is_local)) {
        mpi_errno = MPIDI_SHM_part_start(request);
    } else {
        mpi_errno = MPIDI_NM_part_start(request);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Pready_range(int partition_low, int partition_high,
                                               MPIR_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_pready_range(partition_low, partition_high, request);
#else
    if (MPIDI_REQUEST(request, is_local)) {
        mpi_errno = MPIDI_SHM_mpi_pready_range(partition_low, partition_high, request);
    } else {
        mpi_errno = MPIDI_NM_mpi_pready_range(partition_low, partition_high, request);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Pready_list(int length, const int array_of_partitions[],
                                              MPIR_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_pready_list(length, array_of_partitions, request);
#else
    if (MPIDI_REQUEST(request, is_local)) {
        mpi_errno = MPIDI_SHM_mpi_pready_list(length, array_of_partitions, request);
    } else {
        mpi_errno = MPIDI_NM_mpi_pready_list(length, array_of_partitions, request);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Parrived(MPIR_Request * request, int partition, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_parrived(request, partition, flag);
#else
    if (MPIDI_REQUEST(request, is_local)) {
        mpi_errno = MPIDI_SHM_mpi_parrived(request, partition, flag);
    } else {
        mpi_errno = MPIDI_NM_mpi_parrived(request, partition, flag);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* CH4_PART_H_INCLUDED */
