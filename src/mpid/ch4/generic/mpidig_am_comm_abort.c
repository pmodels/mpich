#include "mpidimpl.h"

static int MPIDIG_comm_abort_origin_cb(MPIR_Request * sreq);
static int MPIDIG_comm_abort_target_msg_cb(int handler_id, void *am_hdr,
                                           void **data, size_t * p_data_sz,
                                           int is_local, int *is_contig,
                                           MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                           MPIR_Request ** req);

int MPIDIG_am_comm_abort_init(void)
{
    return MPIDIG_am_reg_cb(MPIDIG_COMM_ABORT,
                            &MPIDIG_comm_abort_origin_cb, &MPIDIG_comm_abort_target_msg_cb);
}

struct am_comm_abort_hdr {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
};

int MPIDIG_comm_abort(MPIR_Comm * comm, int exit_code)
{
    int mpi_errno = MPI_SUCCESS;
    int dest;
    int size = 0;
    MPIR_Request *sreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMM_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMM_ABORT);

    struct am_comm_abort_hdr am_hdr;
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

static int MPIDIG_comm_abort_origin_cb(MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMM_ABORT_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMM_ABORT_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_COMM_ABORT_ORIGIN_CB);
    return MPI_SUCCESS;
}

static int MPIDIG_comm_abort_target_msg_cb(int handler_id, void *am_hdr,
                                           void **data, size_t * p_data_sz,
                                           int is_local, int *is_contig,
                                           MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                           MPIR_Request ** req)
{
    struct am_comm_abort_hdr *hdr = am_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_COMM_ABORT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_COMM_ABORT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_COMM_ABORT_TARGET_MSG_CB);
    /* looks like we only need tag */
    MPL_exit(hdr->tag);
    return MPI_SUCCESS;
}
