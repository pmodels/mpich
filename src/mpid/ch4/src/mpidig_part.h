/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_H_INCLUDED
#define MPIDIG_AM_PART_H_INCLUDED

#include "ch4_impl.h"
#include "mpidig_part_utils.h"

void MPIDIG_precv_matched(MPIR_Request * part_req);
int MPIDIG_mpi_psend_init(const void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int dest, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request);
int MPIDIG_mpi_precv_init(void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int source, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request);

MPL_STATIC_INLINE_PREFIX int MPIDIG_part_start(MPIR_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Indicate data transfer starts.
     * Decrease when am request completes on sender (via completion_notification),
     * or received data transfer AM on receiver. */
    MPIR_cc_set(request->cc_ptr, 1);

    /* No need to increase refcnt for comm and datatype objects,
     * because it is erroneous to free an active partitioned req if it is not complete.*/

    if (request->kind == MPIR_REQUEST_KIND__PART_RECV && MPIDIG_PART_REQUEST(request, peer_req_ptr)) {
        mpi_errno = MPIDIG_part_issue_cts(request);
    }

    MPIR_Part_request_activate(request);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_pready_range(int partition_low, int partition_high,
                                                     MPIR_Request * part_sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int incomplete = 1, i;
    for (i = partition_low; i <= partition_high; i++)
        MPIR_cc_decr(&MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr, &incomplete);

    if (!incomplete) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
        mpi_errno = MPIDIG_part_issue_data(part_sreq, MPIDIG_PART_REGULAR);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_pready_list(int length, const int array_of_partitions[],
                                                    MPIR_Request * part_sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int i, incomplete = 1;
    for (i = 0; i < length; i++)
        MPIR_cc_decr(&MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr, &incomplete);

    if (!incomplete) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
        mpi_errno = MPIDIG_part_issue_data(part_sreq, MPIDIG_PART_REGULAR);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_parrived(MPIR_Request * request, int partition, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Do not maintain per-partition completion. Arrived when full data transfer is done.
     * An inactive request returns TRUE (same for NULL req, handled at MPIR layer). */
    if (MPIR_Request_is_complete(request)) {
        *flag = TRUE;
    } else {
        *flag = FALSE;

        /* Trigger progress to process AM packages in case wait with parrived in a loop. */
        mpi_errno = MPIDI_progress_test_vci(0);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_AM_PART_H_INCLUDED */
