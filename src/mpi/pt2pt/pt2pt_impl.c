/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Sendrecv_replace_impl(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag,
                               int source, int recvtag, MPIR_Comm * comm_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    void *tmpbuf = NULL;
    MPI_Aint tmpbuf_size = 0;
    MPI_Aint actual_pack_bytes = 0;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SENDRECV_REPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SENDRECV_REPLACE);

    if (count > 0 && dest != MPI_PROC_NULL) {
        MPIR_Pack_size_impl(count, datatype, &tmpbuf_size);

        MPIR_CHKLMEM_MALLOC_ORJUMP(tmpbuf, void *, tmpbuf_size, mpi_errno,
                                   "temporary send buffer", MPL_MEM_BUFFER);

        mpi_errno =
            MPIR_Typerep_pack(buf, count, datatype, 0, tmpbuf, tmpbuf_size, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* If source is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(source == MPI_PROC_NULL)) {
        rreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Status_set_procnull(&rreq->status);
    } else {
        mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag,
                               comm_ptr, MPIR_CONTEXT_INTRA_PT2PT, &rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }


    /* If dest is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(dest == MPI_PROC_NULL)) {
        sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        mpi_errno = MPID_Isend(tmpbuf, actual_pack_bytes, MPI_PACKED, dest,
                               sendtag, comm_ptr, MPIR_CONTEXT_INTRA_PT2PT, &sreq);
        if (mpi_errno != MPI_SUCCESS) {
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPIX_ERR_NOREQ)
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
            /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
            MPIR_Request_free(rreq);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
    }

    mpi_errno = MPID_Wait(rreq, MPI_STATUS_IGNORE);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPID_Wait(sreq, MPI_STATUS_IGNORE);
    MPIR_ERR_CHECK(mpi_errno);

    if (status != MPI_STATUS_IGNORE) {
        *status = rreq->status;
    }

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = rreq->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
        }
    }

    MPIR_Request_free(sreq);
    MPIR_Request_free(rreq);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SENDRECV_REPLACE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
