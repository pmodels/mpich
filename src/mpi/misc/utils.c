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

int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype)
{
    int mpi_errno = MPI_SUCCESS;
    int sendtype_iscontig, recvtype_iscontig;
    MPI_Aint sendsize, recvsize, sdata_sz, rdata_sz, copy_sz;
    MPI_Aint true_extent, sendtype_true_lb, recvtype_true_lb;
    MPIR_CHKLMEM_DECL(1);

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
    if ((HANDLE_IS_BUILTIN(sendtype)) && HANDLE_IS_BUILTIN(recvtype)) {
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
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack((char *) sendbuf + sendtype_true_lb, copy_sz, recvbuf, recvcount,
                            recvtype, 0, &actual_unpack_bytes);
        MPIR_ERR_CHKANDJUMP(actual_unpack_bytes != copy_sz, mpi_errno, MPI_ERR_TYPE,
                            "**dtypemismatch");
    } else if (recvtype_iscontig) {
        MPI_Aint actual_pack_bytes;
        MPIR_Typerep_pack(sendbuf, sendcount, sendtype, 0, (char *) recvbuf + recvtype_true_lb,
                          copy_sz, &actual_pack_bytes);
        MPIR_ERR_CHKANDJUMP(actual_pack_bytes != copy_sz, mpi_errno, MPI_ERR_TYPE,
                            "**dtypemismatch");
    } else {
        char *buf;
        intptr_t sfirst;
        intptr_t rfirst;

        MPIR_CHKLMEM_MALLOC(buf, char *, COPY_BUFFER_SZ, mpi_errno, "buf", MPL_MEM_BUFFER);

        sfirst = 0;
        rfirst = 0;

        while (1) {
            MPI_Aint max_pack_bytes;
            if (copy_sz - sfirst > COPY_BUFFER_SZ) {
                max_pack_bytes = COPY_BUFFER_SZ;
            } else {
                max_pack_bytes = copy_sz - sfirst;
            }

            MPI_Aint actual_pack_bytes;
            MPIR_Typerep_pack(sendbuf, sendcount, sendtype, sfirst, buf,
                              max_pack_bytes, &actual_pack_bytes);
            MPIR_Assert(actual_pack_bytes > 0);

            sfirst += actual_pack_bytes;

            MPI_Aint actual_unpack_bytes;
            MPIR_Typerep_unpack(buf, actual_pack_bytes, recvbuf, recvcount, recvtype,
                                rfirst, &actual_unpack_bytes);
            MPIR_Assert(actual_unpack_bytes > 0);

            rfirst += actual_unpack_bytes;

            if (rfirst == copy_sz) {
                /* successful completion */
                break;
            }

            /* if the send side finished, but the recv side couldn't unpack it, there's a datatype mismatch */
            MPIR_ERR_CHKANDJUMP(sfirst == copy_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
