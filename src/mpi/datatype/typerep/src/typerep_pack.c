/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include <dataloop.h>

int MPIR_Typerep_pack(const void *inbuf, size_t incount, MPI_Datatype datatype,
                      size_t inoffset, void *outbuf, size_t max_pack_bytes,
                      size_t * actual_pack_bytes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Segment *segp;
    int contig;
    size_t dt_true_lb;
    size_t data_sz;
    size_t last;

    if (incount == 0) {
        *actual_pack_bytes = 0;
        goto fn_exit;
    }

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
        contig = TRUE;
        dt_true_lb = 0;
        data_sz = incount * MPIR_Datatype_get_basic_size(datatype);
    } else {
        MPIR_Datatype *dt_ptr;
        MPIR_Datatype_get_ptr(datatype, dt_ptr);
        MPIR_Datatype_is_contig(datatype, &contig);
        dt_true_lb = dt_ptr->true_lb;
        data_sz = incount * dt_ptr->size;
    }

    /* make sure we don't pack more than the max bytes */
    /* FIXME: we need to make sure to pack each basic datatype
     * atomically, even if the max_pack_bytes allows us to split it */
    if (data_sz > max_pack_bytes)
        data_sz = max_pack_bytes;

    /* Handle contig case quickly */
    if (contig) {
        MPIR_Memcpy(outbuf, (char *) inbuf + dt_true_lb + inoffset, data_sz);
        *actual_pack_bytes = data_sz;
    } else {
        segp = MPIR_Segment_alloc(inbuf, incount, datatype);
        MPIR_ERR_CHKANDJUMP1(segp == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Segment");

        last = inoffset + max_pack_bytes;
        MPIR_Segment_pack(segp, inoffset, &last, outbuf);
        MPIR_Segment_free(segp);

        *actual_pack_bytes = last - inoffset;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_unpack(const void *inbuf, size_t insize,
                        void *outbuf, size_t outcount, MPI_Datatype datatype, size_t outoffset,
                        size_t * actual_unpack_bytes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Segment *segp;
    int contig;
    size_t last;
    size_t data_sz;
    size_t dt_true_lb;

    if (insize == 0) {
        *actual_unpack_bytes = 0;
        goto fn_exit;
    }

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
        contig = TRUE;
        dt_true_lb = 0;
        data_sz = outcount * MPIR_Datatype_get_basic_size(datatype);
    } else {
        MPIR_Datatype *dt_ptr;
        MPIR_Datatype_get_ptr(datatype, dt_ptr);
        MPIR_Datatype_is_contig(datatype, &contig);
        dt_true_lb = dt_ptr->true_lb;
        data_sz = outcount * dt_ptr->size;
    }

    /* make sure we are not unpacking more than the provided buffer
     * length */
    /* FIXME: we need to make sure to unpack each basic datatype
     * atomically, even if the max_unpack_bytes allows us to split
     * it */
    if (data_sz > insize)
        data_sz = insize;

    /* Handle contig case quickly */
    if (contig) {
        MPIR_Memcpy((char *) outbuf + dt_true_lb + outoffset, inbuf, data_sz);
        *actual_unpack_bytes = data_sz;
    } else {
        segp = MPIR_Segment_alloc(outbuf, outcount, datatype);
        MPIR_ERR_CHKANDJUMP1(segp == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Segment_alloc");

        last = data_sz + outoffset;
        MPIR_Segment_unpack(segp, outoffset, &last, inbuf);
        MPIR_Segment_free(segp);

        *actual_unpack_bytes = last - outoffset;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
