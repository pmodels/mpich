/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_SEND_H_INCLUDED
#define MPIDIG_SEND_H_INCLUDED

#include "ch4_impl.h"

/* MPIDIG_AM_SEND message uses flags in MPIDIG_hdr_t to mark different mode.
 * The bits are masked as following:
 *     0-7  individual flags as defined below
 *     8-15 RNDV ID if MPIDIG_AM_SEND_FLAGS_RTS is set
 *     16-  reserved, always zero.
 */

#define MPIDIG_AM_SEND_FLAGS_NONE (0)
#define MPIDIG_AM_SEND_FLAGS_SYNC (1)
#define MPIDIG_AM_SEND_FLAGS_RTS (1 << 1)

#define MPIDIG_AM_SEND_SET_RNDV(flags, id) \
    do { \
        flags = (flags & 0xff) | MPIDIG_AM_SEND_FLAGS_RTS | (id << 8); \
    } while (0)

#define MPIDIG_AM_SEND_GET_RNDV_ID(flags) (flags >> 8)

MPL_STATIC_INLINE_PREFIX bool MPIDIG_check_eager(int is_local, MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                 const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, MPIR_Request * sreq)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    return MPIDI_NM_am_check_eager(am_hdr_sz, data_sz, buf, count, datatype, sreq);
#else
    if (is_local) {
        return MPIDI_SHM_am_check_eager(am_hdr_sz, data_sz, buf, count, datatype, sreq);
    } else {
        return MPIDI_NM_am_check_eager(am_hdr_sz, data_sz, buf, count, datatype, sreq);
    }
#endif
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_isend_impl(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank, int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, uint8_t flags,
                                               int src_vci, int dst_vci,
                                               MPIR_Request ** request, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;

    MPIR_FUNC_ENTER;

    if (*request == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2, src_vci, dst_vci);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        *request = sreq;
        sreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    } else {
        sreq = *request;
        MPIDIG_request_init(sreq, src_vci, dst_vci);
        if (!sreq->comm) {
            sreq->comm = comm;
            MPIR_Comm_add_ref(comm);
        }
    }

    MPIDIG_hdr_t am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;
    am_hdr.sreq_ptr = sreq;
    am_hdr.flags = flags;
    if (flags & MPIDIG_AM_SEND_FLAGS_SYNC) {
        MPIR_cc_inc(sreq->cc_ptr);      /* expecting SSEND_ACK */
    }
    MPI_Aint data_sz;
    MPIDI_Datatype_check_size(datatype, count, data_sz);
    am_hdr.data_sz = data_sz;
    am_hdr.rndv_hdr_sz = 0;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

    int is_local = MPIDI_av_is_local(addr);
    MPI_Aint am_hdr_sz = (MPI_Aint) sizeof(am_hdr);
    if (MPIDIG_check_eager(is_local, am_hdr_sz, data_sz, buf, count, datatype, sreq)) {
        /* EAGER send */
        CH4_CALL(am_isend(rank, comm, MPIDIG_SEND, &am_hdr, am_hdr_sz, buf, count, datatype,
                          src_vci, dst_vci, sreq), is_local, mpi_errno);
    } else {
        /* RNDV send */
        MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
        MPIDIG_REQUEST(sreq, count) = count;
        MPIDIG_REQUEST(sreq, datatype) = datatype;
        MPIDIG_REQUEST(sreq, u.send.dest) = rank;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIDIG_AM_SEND_SET_RNDV(am_hdr.flags, MPIDIG_RNDV_GENERIC);

        CH4_CALL(am_send_hdr(rank, comm, MPIDIG_SEND, &am_hdr, am_hdr_sz, src_vci, dst_vci),
                 is_local, mpi_errno);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_isend(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * addr, int src_vci, int dst_vci,
                                              MPIR_Request ** request,
                                              bool syncflag, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    uint8_t flags = syncflag ? MPIDIG_AM_SEND_FLAGS_SYNC : MPIDIG_AM_SEND_FLAGS_NONE;
    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  flags, src_vci, dst_vci, request, errflag);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* cannot cancel send */

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* MPIDIG_SEND_H_INCLUDED */
