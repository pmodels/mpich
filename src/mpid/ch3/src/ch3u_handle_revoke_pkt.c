/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Revoke
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Revoke(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                                MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    MPIDI_CH3_Pkt_revoke_t *revoke_pkt = &pkt->revoke;
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;

    MPIU_DBG_MSG_D(CH3_OTHER, VERBOSE, "Received revoke pkt from %d", vc->pg_rank);

    /* Search through all of the communicators to find the right context_id */
    MPIDI_CH3I_Comm_find(revoke_pkt->revoked_comm, &comm_ptr);
    if (comm_ptr == NULL)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                "**ch3|postrecv %s", "MPIDI_CH3_PKT_REVOKE");

    mpi_errno = MPID_Comm_revoke(comm_ptr, 1);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                "**ch3|postrecv %s", "MPIDI_CH3_PKT_REVOKE");

    /* There is no request associated with a revoke packet */
    *rreqp = NULL;

fn_fail:
    return mpi_errno;
}
