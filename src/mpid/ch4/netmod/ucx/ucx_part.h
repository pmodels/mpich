/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PART_H_INCLUDED
#define UCX_PART_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_part_start(MPIR_Request * request)
{
    return MPIDIG_part_start(request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_pready_range(int partition_low,
                                                       int partition_high, MPIR_Request * request)
{
    return MPIDIG_mpi_pready_range(partition_low, partition_high, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_pready_list(int length, const int array_of_partitions[],
                                                      MPIR_Request * request)
{
    return MPIDIG_mpi_pready_list(length, array_of_partitions, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_parrived(MPIR_Request * request, int partition, int *flag)
{
    return MPIDIG_mpi_parrived(request, partition, flag);
}

#endif /* UCX_PART_H_INCLUDED */
