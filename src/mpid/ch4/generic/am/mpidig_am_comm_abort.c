/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"

static int comm_abort_origin_cb(MPIR_Request * sreq);
static int comm_abort_target_msg_cb(void *am_hdr,
                                    void *data, MPI_Aint data_sz, uint32_t attr,
                                    MPIR_Request ** req);

void MPIDIG_am_comm_abort_init(void)
{
    MPIDIG_am_reg_cb(MPIDIG_COMM_ABORT, &comm_abort_origin_cb, &comm_abort_target_msg_cb);
}

/* MPIDIG_COMM_ABORT */

struct am_comm_abort_hdr {
    int exit_code;
};

int MPIDIG_am_comm_abort(MPIR_Comm * comm, int exit_code)
{
    int mpi_errno = MPI_SUCCESS;
    int dest;
    int size = 0;
    MPIR_Request *sreq = NULL;

    MPIR_FUNC_ENTER;

    struct am_comm_abort_hdr am_hdr;
    am_hdr.exit_code = exit_code;

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        size = comm->local_size;
    } else {
        size = comm->remote_size;
    }

    for (dest = 0; dest < size; dest++) {
        if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM && dest == comm->rank)
            continue;

        /* 2 references, 1 for MPID-layer and 1 for MPIR-layer */
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2, 0, 0);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        /* FIXME: only NM? */
        mpi_errno = MPIDI_NM_am_isend(dest, comm, MPIDIG_COMM_ABORT, &am_hdr,
                                      sizeof(am_hdr), NULL, 0, MPI_INT, 0, 0, sreq);
        if (mpi_errno)
            continue;
        else
            MPIR_Wait_impl(sreq, MPI_STATUSES_IGNORE);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPL_exit(exit_code);

    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

static int comm_abort_origin_cb(MPIR_Request * sreq)
{
    return MPID_Request_complete(sreq);
}

static int comm_abort_target_msg_cb(void *am_hdr,
                                    void *data, MPI_Aint data_sz, uint32_t attr,
                                    MPIR_Request ** req)
{
    MPIR_FUNC_ENTER;

    struct am_comm_abort_hdr *hdr = am_hdr;

    MPL_exit(hdr->exit_code);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}
