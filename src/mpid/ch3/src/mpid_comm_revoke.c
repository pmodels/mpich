/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   (C) 2014 by Argonne National Laboratory.
 *       See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * This function does all of the work or either revoking the communciator for
 * the first time or keeping track of an ongoing revocation.
 *
 * comm_ptr  - The communicator being revoked
 * is_remote - If we received the revocation from a remote process, this should
 *             be set to true. This way we'll know to decrement the counter twice
 *             (once for our local revocation and once for the remote).
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_revoke
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_revoke(MPID_Comm *comm_ptr, int is_remote)
{
    MPIDI_VC_t *vc;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;
    int i, size, my_rank;
    MPID_Request *request;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_revoke_t *revoke_pkt = &upkt.revoke;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_REVOKE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_REVOKE);

    if (0 == comm_ptr->revoked) {
        /* Mark the communicator as revoked locally */
        comm_ptr->revoked = 1;
        if (comm_ptr->node_comm) comm_ptr->node_comm->revoked = 1;
        if (comm_ptr->node_roots_comm) comm_ptr->node_roots_comm->revoked = 1;

        /* Start a counter to track how many revoke messages we've received from
         * other ranks */
        comm_ptr->dev.waiting_for_revoke = comm_ptr->local_size - 1 - is_remote; /* Subtract the processes who already know about the revoke */
        MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE, (MPIU_DBG_FDEST, "Comm %08x waiting_for_revoke: %d", comm_ptr->handle, comm_ptr->dev.waiting_for_revoke));

        /* Keep a reference to this comm so it doesn't get destroyed while
         * it's being revoked */
        MPIR_Comm_add_ref(comm_ptr);

        /* Send out the revoke message */
        MPIDI_Pkt_init(revoke_pkt, MPIDI_CH3_PKT_REVOKE);
        revoke_pkt->revoked_comm = comm_ptr->context_id;

        size = comm_ptr->remote_size;
        my_rank = comm_ptr->rank;
        for (i = 0; i < size; i++) {
            if (i == my_rank) continue;
            request = NULL;

            MPIDI_Comm_get_vc_set_active(comm_ptr, i, &vc);

            iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) revoke_pkt;
            iov[0].MPL_IOV_LEN = sizeof(*revoke_pkt);

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 1, &request);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            if (mpi_errno) comm_ptr->dev.waiting_for_revoke--;
            if (NULL != request)
                /* We don't need to keep a reference to this request. The
                 * progress engine will keep a reference until it completes
                 * later */
                MPID_Request_release(request);
        }

        /* Check to see if we are done revoking */
        if (comm_ptr->dev.waiting_for_revoke == 0) {
            MPIR_Comm_release(comm_ptr);
        }

        /* Go clean up all of the existing operations involving this
         * communicator. This includes completing existing MPI requests, MPID
         * requests, and cleaning up the unexpected queue to make sure there
         * aren't any unexpected messages hanging around. */

        /* Clean up the receive and unexpected queues */
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
        MPIDI_CH3U_Clean_recvq(comm_ptr);
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    } else if (is_remote)  { /* If this is local, we've already revoked and don't need to do it again. */
        /* Decrement the revoke counter */
        comm_ptr->dev.waiting_for_revoke--;
        MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE, (MPIU_DBG_FDEST, "Comm %08x waiting_for_revoke: %d", comm_ptr->handle, comm_ptr->dev.waiting_for_revoke));

        /* Check to see if we are done revoking */
        if (comm_ptr->dev.waiting_for_revoke == 0) {
            MPIR_Comm_release(comm_ptr);
        }
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_REVOKE);
    return MPI_SUCCESS;
}
