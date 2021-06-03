/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_H_INCLUDED
#define MPIDIG_AM_PART_H_INCLUDED

#include "ch4_impl.h"
#include "mpidig_am_part_utils.h"

int MPIDIG_mpi_psend_init(void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int dest, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, int is_local,
                          MPIR_Request ** request);
int MPIDIG_mpi_precv_init(void *buf, int partitions, int count,
                          MPI_Datatype datatype, int source, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, int is_local,
                          MPIR_Request ** request);

MPL_STATIC_INLINE_PREFIX int MPIDIG_part_start(MPIR_Request * request, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_START);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    int status = MPIDIG_PART_REQ_INC_FETCH_STATUS(request);

    /* Indicate data transfer starts.
     * Decrease when am request completes on sender (via completion_notification),
     * or received data transfer AM on receiver. */
    MPIR_cc_set(request->cc_ptr, 1);
    if (request->kind == MPIR_REQUEST_KIND__PART_SEND) {
        MPIR_cc_set(&MPIDIG_PART_REQUEST(request, u.send).ready_cntr, 0);
    }

    /* No need to increase refcnt for comm and datatype objects,
     * because it is erroneous to free an active partitioned req if it is not complete.*/

    if (request->kind == MPIR_REQUEST_KIND__PART_RECV && status == MPIDIG_PART_REQ_CTS) {
        mpi_errno = MPIDIG_part_issue_cts(request);
    }

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_START);
    return mpi_errno;
}

/* Checks whether all partitions are ready and MPIDIG_PART_REQ_CTS has been received,
 * If so, then issues data. Otherwise a no-op.
 */
MPL_STATIC_INLINE_PREFIX int MPIDIG_post_pready(MPIR_Request * part_sreq, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_POST_PREADY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_POST_PREADY);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Send data when all partitions are ready */
    if (MPIR_cc_get(MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr) ==
        part_sreq->u.part.partitions &&
        MPL_atomic_load_int(&MPIDIG_PART_REQUEST(part_sreq, status)) == MPIDIG_PART_REQ_CTS) {
        mpi_errno = MPIDIG_part_issue_data(part_sreq, is_local);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_POST_PREADY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_pready_range(int partition_low, int partition_high,
                                                     MPIR_Request * part_sreq, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_PREADY_RANGE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_PREADY_RANGE);

    int c = 0, i;
    for (i = partition_low; i <= partition_high; i++)
        MPIR_cc_incr(&MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr, &c);
    MPIR_Assert(MPIR_cc_get(MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr) <=
                part_sreq->u.part.partitions);

    mpi_errno = MPIDIG_post_pready(part_sreq, is_local);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_PREADY_RANGE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_pready_list(int length, int array_of_partitions[],
                                                    MPIR_Request * part_sreq, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_PREADY_LIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_PREADY_LIST);

    int i, c = 0;
    for (i = 0; i < length; i++)
        MPIR_cc_incr(&MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr, &c);
    MPIR_Assert(MPIR_cc_get(MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr) <=
                part_sreq->u.part.partitions);

    mpi_errno = MPIDIG_post_pready(part_sreq, is_local);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_PREADY_LIST);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_parrived(MPIR_Request * request, int partition, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_PARRIVED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_PARRIVED);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Do not maintain per-partition completion. Arrived when full data transfer is done.
     * An inactive request returns TRUE (same for NULL req, handled at MPIR layer). */
    if (MPIR_Request_is_complete(request)) {
        *flag = TRUE;
    } else {
        *flag = FALSE;

        /* Trigger progress to process AM packages in case wait with parrived in a loop. */
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_PARRIVED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_AM_PART_H_INCLUDED */
