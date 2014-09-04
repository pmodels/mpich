/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Comm_failure_ack
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_FAILURE_ACK);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_failure_get_acked
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
#define FUNCNAME MPID_Comm_failed_bitarray
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Comm_failed_bitarray(MPID_Comm *comm_ptr, uint32_t **bitarray, int acked)
{
    int mpi_errno = MPI_SUCCESS;
    int size, i;
    uint32_t bit;
    int *failed_procs, *group_procs;
    MPID_Group *failed_group, *comm_group;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_COMM_FAILED_BITARRAY);

    MPIDI_FUNC_ENTER(MPID_STATE_COMM_FAILED_BITARRAY);

    /* TODO - Fix this for intercommunicators */
    size = comm_ptr->local_size;

    /* We can fit sizeof(uint32_t) * 8 ranks in one uint64_t so divide the
     * size by that */
    /* This buffer will be handed back to the calling function so we use a
     * "real" malloc here and expect the caller to free the buffer later. The
     * other buffers in this function are temporary and will be automatically
     * cleaned up at the end of the function. */
    *bitarray = (uint32_t *) MPIU_Malloc(sizeof(uint32_t) * (size / (sizeof(uint32_t) * 8)+1));
    if (!(*bitarray)) {
        fprintf(stderr, "Could not allocate space for bitarray\n");
        PMPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i <= size/(sizeof(uint32_t)*8); i++) *bitarray[i] = 0;

    mpi_errno = MPIDI_CH3U_Check_for_failed_procs();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (acked)
        MPIDI_CH3U_Get_failed_group(comm_ptr->dev.last_ack_rank, &failed_group);
    else
        MPIDI_CH3U_Get_failed_group(-2, &failed_group);

    if (failed_group == MPID_Group_empty) goto fn_exit;


    MPIU_CHKLMEM_MALLOC(group_procs, int *, sizeof(int)*failed_group->size, mpi_errno, "group_procs");
    for (i = 0; i < failed_group->size; i++) group_procs[i] = i;
    MPIU_CHKLMEM_MALLOC(failed_procs, int *, sizeof(int)*failed_group->size, mpi_errno, "failed_procs");

    MPIR_Comm_group_impl(comm_ptr, &comm_group);

    MPIR_Group_translate_ranks_impl(failed_group, failed_group->size, group_procs, comm_group, failed_procs);

    /* The bits will actually be ordered in decending order rather than
     * ascending. This is purely for readability since it makes no practical
     * difference. So if the bits look like this:
     *
     * 10001100 01001000 00000000 00000001
     *
     * Then processes 1, 5, 6, 9, 12, and 32 have failed. */
    for (i = 0; i < failed_group->size; i++) {
        bit = 0x80000000;
        bit >>= failed_procs[i] % (sizeof(uint32_t) * 8);

        *bitarray[failed_procs[i] / (sizeof(uint32_t) * 8)] |= bit;
    }

    MPIR_Group_free_impl(comm_group);

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
