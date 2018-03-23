/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"
#include "datatype.h"

#define COPY_BUFFER_SZ 16384

#undef FUNCNAME
#define FUNCNAME MPIR_Localcopy
#undef FCNAME
#define FCNAME "MPIR_Localcopy"
int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype)
{
    int mpi_errno = MPI_SUCCESS;
    int sendtype_iscontig, recvtype_iscontig;
    MPI_Aint sendsize, recvsize, sdata_sz, rdata_sz, copy_sz;
    MPI_Aint true_extent, sendtype_true_lb, recvtype_true_lb;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_LOCALCOPY);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_LOCALCOPY);

    MPIR_Datatype_get_size_macro(sendtype, sendsize);
    MPIR_Datatype_get_size_macro(recvtype, recvsize);

    sdata_sz = sendsize * sendcount;
    rdata_sz = recvsize * recvcount;

    /* if there is no data to copy, bail out */
    if (!sdata_sz || !rdata_sz)
        goto fn_exit;

#if defined(HAVE_ERROR_CHECKING)
    if (sdata_sz > rdata_sz) {
        MPIR_ERR_SET2(mpi_errno, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz,
                      rdata_sz);
        copy_sz = rdata_sz;
    } else
#endif /* HAVE_ERROR_CHECKING */
        copy_sz = sdata_sz;

    /* Builtin types is the common case; optimize for it */
    if ((HANDLE_GET_KIND(sendtype) == HANDLE_KIND_BUILTIN) &&
        HANDLE_GET_KIND(recvtype) == HANDLE_KIND_BUILTIN) {
        MPIR_Memcpy(recvbuf, sendbuf, copy_sz);
        goto fn_exit;
    }

    MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
    MPIR_Datatype_iscontig(recvtype, &recvtype_iscontig);

    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &true_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &true_extent);

    if (sendtype_iscontig && recvtype_iscontig) {
#if defined(HAVE_ERROR_CHECKING)
        MPIR_ERR_CHKMEMCPYANDJUMP(mpi_errno,
                                  ((char *) recvbuf + recvtype_true_lb),
                                  ((char *) sendbuf + sendtype_true_lb), copy_sz);
#endif
        MPIR_Memcpy(((char *) recvbuf + recvtype_true_lb),
                    ((char *) sendbuf + sendtype_true_lb), copy_sz);
    } else if (sendtype_iscontig) {
        MPIR_Segment seg;
        MPI_Aint last;

        MPIR_Segment_init(recvbuf, recvcount, recvtype, &seg);
        last = copy_sz;
        MPIR_Segment_unpack(&seg, 0, &last, (char *) sendbuf + sendtype_true_lb);
        MPIR_ERR_CHKANDJUMP(last != copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
    } else if (recvtype_iscontig) {
        MPIR_Segment seg;
        MPI_Aint last;

        MPIR_Segment_init(sendbuf, sendcount, sendtype, &seg);
        last = copy_sz;
        MPIR_Segment_pack(&seg, 0, &last, (char *) recvbuf + recvtype_true_lb);
        MPIR_ERR_CHKANDJUMP(last != copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
    } else {
        char *buf;
        intptr_t buf_off;
        MPIR_Segment sseg;
        intptr_t sfirst;
        MPIR_Segment rseg;
        intptr_t rfirst;

        MPIR_CHKLMEM_MALLOC(buf, char *, COPY_BUFFER_SZ, mpi_errno, "buf", MPL_MEM_BUFFER);

        MPIR_Segment_init(sendbuf, sendcount, sendtype, &sseg);
        MPIR_Segment_init(recvbuf, recvcount, recvtype, &rseg);

        sfirst = 0;
        rfirst = 0;
        buf_off = 0;

        while (1) {
            MPI_Aint last;
            char *buf_end;

            if (copy_sz - sfirst > COPY_BUFFER_SZ - buf_off) {
                last = sfirst + (COPY_BUFFER_SZ - buf_off);
            } else {
                last = copy_sz;
            }

            MPIR_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
            MPIR_Assert(last > sfirst);

            buf_end = buf + buf_off + (last - sfirst);
            sfirst = last;

            MPIR_Segment_unpack(&rseg, rfirst, &last, buf);
            MPIR_Assert(last > rfirst);

            rfirst = last;

            if (rfirst == copy_sz) {
                /* successful completion */
                break;
            }

            /* if the send side finished, but the recv side couldn't unpack it, there's a datatype mismatch */
            MPIR_ERR_CHKANDJUMP(sfirst == copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");

            /* if not all data was unpacked, copy it to the front of the buffer for next time */
            buf_off = sfirst - rfirst;
            if (buf_off > 0) {
                memmove(buf, buf_end - buf_off, buf_off);
            }
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_LOCALCOPY);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
