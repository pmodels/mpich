/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* Common internal routine for send_init family */
static int psend_init(MPIDI_ptype ptype,
                      const void *buf,
                      MPI_Aint count,
                      MPI_Datatype datatype,
                      int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vci = MPIDI_get_vci(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    MPIDI_CH4_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__PREQUEST_SEND, vci, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;

    MPIDI_PREQUEST(sreq, p_type) = ptype;
    MPIDI_PREQUEST(sreq, buffer) = (void *) buf;
    MPIDI_PREQUEST(sreq, count) = count;
    MPIDI_PREQUEST(sreq, datatype) = datatype;
    MPIDI_PREQUEST(sreq, rank) = rank;
    MPIDI_PREQUEST(sreq, tag) = tag;
    MPIDI_PREQUEST(sreq, context_offset) = context_offset;
    sreq->u.persist.real_request = NULL;
    MPIR_cc_set(sreq->cc_ptr, 0);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Send_init(const void *buf,
                   MPI_Aint count,
                   MPI_Datatype datatype,
                   int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = psend_init(MPIDI_PTYPE_SEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Ssend_init(const void *buf,
                    MPI_Aint count,
                    MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_PT2PT_ATTR_SET_SYNCFLAG(attr);
    mpi_errno = psend_init(MPIDI_PTYPE_SEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Bsend_init(const void *buf,
                    MPI_Aint count,
                    MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = psend_init(MPIDI_PTYPE_BSEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Rsend_init(const void *buf,
                    MPI_Aint count,
                    MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* TODO: Currently we don't distinguish SEND and RSEND */
    mpi_errno = psend_init(MPIDI_PTYPE_SEND, buf, count, datatype, rank, tag, comm, attr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

int MPID_Recv_init(void *buf,
                   MPI_Aint count,
                   MPI_Datatype datatype,
                   int rank, int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vci = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    MPIDI_CH4_REQUEST_CREATE(rreq, MPIR_REQUEST_KIND__PREQUEST_RECV, vci, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    *request = rreq;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    MPIDI_PREQUEST(rreq, buffer) = (void *) buf;
    MPIDI_PREQUEST(rreq, count) = count;
    MPIDI_PREQUEST(rreq, datatype) = datatype;
    MPIDI_PREQUEST(rreq, rank) = rank;
    MPIDI_PREQUEST(rreq, tag) = tag;
    MPIDI_PREQUEST(rreq, context_offset) = context_offset;
    rreq->u.persist.real_request = NULL;
    MPIR_cc_set(rreq->cc_ptr, 0);

    MPIDI_PREQUEST(rreq, p_type) = MPIDI_PTYPE_RECV;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
