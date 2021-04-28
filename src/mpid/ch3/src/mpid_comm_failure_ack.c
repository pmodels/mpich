/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPID_Comm_failure_ack(MPIR_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_ACK);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_FAILURE_ACK);

    /* Update the list of failed processes that we know about locally.
     * This part could technically be turned off and be a correct
     * implementation, but it would be slower about propagating failure
     * information. Also, this is the failure case so speed isn't as
     * important. */
    MPIDI_CH3U_Check_for_failed_procs();

    /* Update the marker for the last known failed process in this
     * communciator. */
    comm_ptr->dev.last_ack_rank = MPIDI_last_known_failed;

    /* Mark the communicator as any source active */
    comm_ptr->dev.anysource_enabled = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_FAILURE_ACK);
    return mpi_errno;
}

int MPID_Comm_failure_get_acked(MPIR_Comm *comm_ptr, MPIR_Group **group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *failed_group, *comm_group;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);

    /* Get the group of all failed processes */
    MPIDI_CH3U_Check_for_failed_procs();
    MPIDI_CH3U_Get_failed_group(comm_ptr->dev.last_ack_rank, &failed_group);
    if (failed_group == MPIR_Group_empty) {
        *group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    MPIR_Comm_group_impl(comm_ptr, &comm_group);

    /* Get the intersection of all failed processes in this communicator */
    MPIR_Group_intersection_impl(failed_group, comm_group, group_ptr);

    MPIR_Group_release(comm_group);
    MPIR_Group_release(failed_group);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);
    return mpi_errno;
}
