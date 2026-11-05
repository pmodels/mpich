/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"

static int comm_revoke_origin_cb(MPIR_Request * sreq);
static int comm_revoke_target_msg_cb(void *am_hdr,
                                     void *data, MPI_Aint data_sz, uint32_t attr,
                                     MPIR_Request ** req);

struct am_comm_revoke_hdr {
    int revoked_context_id;
};

static void find_comm_by_context_id(int context_id, MPIR_Comm ** comm)
{
    *comm = MPIDIU_map_lookup(MPIDI_global.comm_req_map, context_id);
    if (*comm == MPIDIU_MAP_NOT_FOUND) {
        *comm = NULL;
    }
}

void MPIDIG_am_comm_revoke_init(void)
{
    MPIDIG_am_reg_cb(MPIDIG_COMM_REVOKE, &comm_revoke_origin_cb, &comm_revoke_target_msg_cb);
}

int MPIDIG_am_comm_revoke(MPIR_Comm * comm, int is_remote)
{
    int mpi_errno = MPI_SUCCESS;
    struct am_comm_revoke_hdr am_hdr;
    MPIR_Request *sreq = NULL;

    MPIR_FUNC_ENTER;

    if (is_remote) {
        goto fn_exit;
    }

    am_hdr.revoked_context_id = comm->context_id;

    for (int dest = 0; dest < comm->local_size; dest++) {
        if (dest == comm->rank) {
            continue;
        }

        MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_REQUEST_POOL_MUTEXES[0]);
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2, 0, 0);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        mpi_errno = MPIDI_NM_am_isend(dest, comm, MPIDIG_COMM_REVOKE, &am_hdr,
                                      sizeof(am_hdr), NULL, 0, MPI_BYTE, 0, 0, sreq);
        MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_REQUEST_POOL_MUTEXES[0]);
        if (mpi_errno) {
            mpi_errno = MPI_SUCCESS;
            continue;
        }
        MPIR_Wait_impl(sreq, MPI_STATUS_IGNORE);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_REQUEST_POOL_MUTEXES[0]);
    goto fn_exit;
}

static int comm_revoke_origin_cb(MPIR_Request * sreq)
{
    return MPID_Request_complete(sreq);
}

static int comm_revoke_target_msg_cb(void *am_hdr,
                                     void *data, MPI_Aint data_sz, uint32_t attr,
                                     MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm = NULL;
    struct am_comm_revoke_hdr *hdr = am_hdr;

    MPIR_FUNC_ENTER;

    find_comm_by_context_id(hdr->revoked_context_id, &comm);
    MPIR_ERR_CHKANDJUMP(comm == NULL, mpi_errno, MPI_ERR_OTHER, "**ch4|postrecv");

    mpi_errno = MPID_Comm_revoke(comm, 1);
    MPIR_ERR_CHECK(mpi_errno);

    if (req) {
        *req = NULL;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
