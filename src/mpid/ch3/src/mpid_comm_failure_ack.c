/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Comm_failure_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_failure_ack(MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_FAILURE_ACK);

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

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_FAILURE_ACK);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_failure_get_acked
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_failure_get_acked(MPID_Comm *comm_ptr, MPID_Group **group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *failed_group, *comm_group;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);

    /* Get the group of all failed processes */
    MPIDI_CH3U_Check_for_failed_procs();
    MPIDI_CH3U_Get_failed_group(comm_ptr->dev.last_ack_rank, &failed_group);
    if (failed_group == MPID_Group_empty) {
        *group_ptr = MPID_Group_empty;
        goto fn_exit;
    }

    MPIR_Comm_group_impl(comm_ptr, &comm_group);

    /* Get the intersection of all falied processes in this communicator */
    MPIR_Group_intersection_impl(failed_group, comm_group, group_ptr);

    MPIR_Group_release(comm_group);
    MPIR_Group_release(failed_group);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_FAILURE_GET_ACKED);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_AS_enabled
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_AS_enabled(MPID_Comm *comm_ptr) {
    return comm_ptr->dev.anysource_enabled;
}


#undef FUNCNAME
#define FUNCNAME MPID_Request_is_anysource
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Request_is_anysource(MPID_Request *request_ptr) {
    int ret = 0;

    if (request_ptr->kind == MPID_REQUEST_RECV)
        ret = request_ptr->dev.match.parts.rank == MPI_ANY_SOURCE;

    return ret;
}
