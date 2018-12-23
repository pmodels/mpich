/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "mpidig.h"
#include "ch4_impl.h"
#include "mpidch4r.h"

#undef FUNCNAME
#define FUNCNAME MPIDIG_comm_abort
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_comm_abort(MPIR_Comm * comm, int exit_code)
{
    int mpi_errno = MPI_SUCCESS;
    int dest;
    int size = 0;
    MPIR_Request *sreq = NULL;
    MPIDIG_hdr_t am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMM_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMM_ABORT);

    am_hdr.src_rank = comm->rank;
    am_hdr.tag = exit_code;
    am_hdr.context_id = comm->context_id + MPIR_CONTEXT_INTRA_PT2PT;

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        size = comm->local_size;
    } else {
        size = comm->remote_size;
    }

    for (dest = 0; dest < size; dest++) {
        if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && dest == comm->rank)
            continue;

        mpi_errno = MPI_SUCCESS;
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        mpi_errno = MPIDI_NM_am_isend(dest, comm, MPIDIG_COMM_ABORT, &am_hdr,
                                      sizeof(am_hdr), NULL, 0, MPI_INT, sreq);
        if (mpi_errno)
            continue;
        else
            MPIR_Wait_impl(sreq, MPI_STATUSES_IGNORE);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_COMM_ABORT);
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPL_exit(exit_code);

    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

#ifdef MPIDI_CH4_ULFM
#undef FUNCNAME
#define FUNCNAME MPIDIG_comm_revoke
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_comm_revoke(MPIR_Comm * comm_ptr, int is_remote)
{
    int mpi_errno = MPI_SUCCESS;
    int dest;
    MPIR_Request *sreq = NULL;
    MPIDIG_hdr_t am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMM_REVOKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMM_REVOKE);

    if (0 == comm_ptr->revoked) {
        /* Mark the communicator as revoked locally */
        comm_ptr->revoked = 1;
        if (comm_ptr->node_comm)
            comm_ptr->node_comm->revoked = 1;
        if (comm_ptr->node_roots_comm)
            comm_ptr->node_roots_comm->revoked = 1;

        /* Start a counter to track how many revoke messages we've received from
         * other ranks */
        MPIDIG_COMM(comm_ptr, waiting_for_revoke) = comm_ptr->local_size - 1 - is_remote;       /* Subtract the processes who already know about the revoke */
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "Comm %08x waiting_for_revoke: %d",
                         comm_ptr->handle, MPIDIG_COMM(comm_ptr, waiting_for_revoke)));

        /* Keep a reference to this comm so it doesn't get destroyed while
         * it's being revoked */
        MPIR_Comm_add_ref(comm_ptr);

        /* Send out the revoke message */
        am_hdr.src_rank = comm_ptr->rank;
        am_hdr.tag = 0;
        am_hdr.context_id = comm_ptr->context_id + MPIR_CONTEXT_INTRA_PT2PT;

        for (dest = 0; dest < comm_ptr->remote_size; dest++) {
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && dest == comm_ptr->rank)
                continue;

            sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, 2);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");

            mpi_errno = MPIDI_NM_am_isend(dest, comm_ptr, MPIDIG_COMM_REVOKE, &am_hdr,
                                          sizeof(am_hdr), NULL, 0, MPI_INT, sreq);
            if (mpi_errno)
                MPIDIG_COMM(comm_ptr, waiting_for_revoke)--;
            else
                MPIR_Wait_impl(sreq, MPI_STATUSES_IGNORE);

            if (NULL != sreq)
                /* We don't need to keep a reference to this request. The
                 * progress engine will keep a reference until it completes
                 * later */
                MPIR_Request_free(sreq);
        }

        /* Check to see if we are done revoking */
        if (MPIDIG_COMM(comm_ptr, waiting_for_revoke) == 0)
            MPIR_Comm_release(comm_ptr);

        /* Go clean up all of the existing operations involving this
         * communicator. This includes completing existing MPI requests, MPID
         * requests, and cleaning up the unexpected queue to make sure there
         * aren't any unexpected messages hanging around. */

        /* Clean up the receive and unexpected queues */
        MPIDIG_recvq_clean(comm_ptr);
    } else if (is_remote) {     /* If this is local, we've already revoked and don't need to do it again. */
        /* Decrement the revoke counter */
        MPIDIG_COMM(comm_ptr, waiting_for_revoke)--;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, "Comm %08x waiting_for_revoke: %d",
                         comm_ptr->handle, MPIDIG_COMM(comm_ptr, waiting_for_revoke)));

        /* Check to see if we are done revoking */
        if (MPIDIG_COMM(comm_ptr, waiting_for_revoke) == 0) {
            MPIR_Comm_release(comm_ptr);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_COMM_REVOKE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
