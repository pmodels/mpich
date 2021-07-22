/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PART_H_INCLUDED
#define UCX_PART_H_INCLUDED

#include "ucx_impl.h"
#include "ucx_part_utils.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_part_start(MPIR_Request * request)
{
    MPIR_Request_add_ref(request);
    MPIR_cc_set(&request->cc, 1);

    /* FIXME: to avoid this we need to modify the noncontig type handling
     * to not free the datatype for persistent/partitioned communication */
    int is_contig;
    MPIR_Datatype_is_contig(MPIDI_PART_REQUEST(request, datatype), &is_contig);
    if (!is_contig) {
        MPIR_Datatype *dt_ptr;

        MPIR_Datatype_get_ptr(MPIDI_PART_REQUEST(request, datatype), dt_ptr);
        MPIR_Assert(dt_ptr != NULL);
        MPIR_Datatype_ptr_add_ref(dt_ptr);
    }

    if (request->kind == MPIR_REQUEST_KIND__PART_RECV &&
        MPIDI_UCX_PART_REQ(request).peer_req != MPI_REQUEST_NULL) {
        MPIDI_UCX_part_recv(request);
    } else if (request->kind == MPIR_REQUEST_KIND__PART_SEND &&
               MPIDI_UCX_PART_REQ(request).peer_req != MPI_REQUEST_NULL) {
        MPIR_cc_set(&MPIDI_UCX_PART_REQ(request).ready_cntr, request->u.part.partitions);
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_pready_range(int partition_low,
                                                       int partition_high, MPIR_Request * request)
{
    int num_ready = partition_high - partition_low + 1;
    int incomplete = 1;

    for (int i = 0; i < num_ready; i++) {
        MPIR_cc_decr(&MPIDI_UCX_PART_REQ(request).ready_cntr, &incomplete);
    }

    if (!incomplete) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
        MPIDI_UCX_part_send(request);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_pready_list(int length, int array_of_partitions[],
                                                      MPIR_Request * request)
{
    int incomplete = 1;

    for (int i = 0; i < length; i++) {
        MPIR_cc_decr(&MPIDI_UCX_PART_REQ(request).ready_cntr, &incomplete);
    }

    if (!incomplete) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
        MPIDI_UCX_part_send(request);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_parrived(MPIR_Request * request, int partition, int *flag)
{
    return MPIDIG_mpi_parrived(request, partition, flag);
}

#endif /* UCX_PART_H_INCLUDED */
