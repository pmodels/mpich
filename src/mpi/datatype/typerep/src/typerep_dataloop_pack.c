/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <dataloop.h>
#include "typerep_pre.h"
#include "typerep_internal.h"

int MPIR_Typerep_test(MPIR_Typerep_req typerep_req, int *completed)
{
    *completed = 1;
    return MPI_SUCCESS;
}

int MPIR_Typerep_icopy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                       MPIR_Typerep_req * typerep_req, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    if (flags & MPIR_TYPEREP_FLAG_STREAM) {
        MPIR_Memcpy_stream(outbuf, inbuf, num_bytes);
    } else {
        MPIR_Memcpy(outbuf, inbuf, num_bytes);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes, uint32_t flags)
{
    return MPIR_Typerep_icopy(outbuf, inbuf, num_bytes, NULL, flags);
}

int MPIR_Typerep_ipack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                       MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                       MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typerep_req, uint32_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Segment *segp;
    int contig;
    MPI_Aint dt_true_lb;
    MPI_Aint data_sz;
    MPI_Aint last;

    if (incount == 0) {
        *actual_pack_bytes = 0;
        goto fn_exit;
    }

    if (HANDLE_IS_BUILTIN(datatype)) {
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

    /* Handle contig case quickly */
    if (contig) {
        MPI_Aint pack_size = data_sz - inoffset;

        /* make sure we don't pack more than the max bytes */
        /* FIXME: we need to make sure to pack each basic datatype
         * atomically, even if the max_pack_bytes allows us to split it */
        pack_size = MPL_MIN(pack_size, max_pack_bytes);

        MPIR_Memcpy(outbuf, MPIR_get_contig_ptr(inbuf, dt_true_lb + inoffset), pack_size);
        *actual_pack_bytes = pack_size;
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

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes, uint32_t flags)
{
    return MPIR_Typerep_ipack(inbuf, incount, datatype, inoffset, outbuf, max_pack_bytes,
                              actual_pack_bytes, NULL, flags);
}


int MPIR_Typerep_iunpack(const void *inbuf, MPI_Aint insize,
                         void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                         MPI_Aint * actual_unpack_bytes, MPIR_Typerep_req * typerep_req,
                         uint32_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Segment *segp;
    int contig;
    MPI_Aint last;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb;

    if (insize == 0) {
        *actual_unpack_bytes = 0;
        goto fn_exit;
    }

    if (HANDLE_IS_BUILTIN(datatype)) {
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
        MPIR_Memcpy(MPIR_get_contig_ptr(outbuf, dt_true_lb + outoffset), inbuf, data_sz);
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

int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize,
                        void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                        MPI_Aint * actual_unpack_bytes, uint32_t flags)
{
    return MPIR_Typerep_iunpack(inbuf, insize, outbuf, outcount, datatype, outoffset,
                                actual_unpack_bytes, NULL, flags);
}

int MPIR_Typerep_wait(MPIR_Typerep_req typerep_req)
{
    /* All nonblocking operations are actually blocking. Thus, do nothing in wait. */
    return MPI_SUCCESS;
}

int MPIR_Typerep_reduce_is_supported(MPI_Op op, MPI_Aint count, MPI_Datatype datatype)
{
    /* This function is supposed to return 1 only for yaksa */
    return 0;
}

int MPIR_Typerep_op(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                    void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp, MPI_Op op,
                    bool source_is_packed, int mapped_device)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* trivial cases */
    if (op == MPI_NO_OP) {
        goto fn_exit;
    }

    /* error checking */
    MPIR_Assert(HANDLE_IS_BUILTIN(op));
    MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));

    mpi_errno = MPII_Typerep_op_fallback(source_buf, source_count, source_dtp,
                                         target_buf, target_count, target_dtp,
                                         op, source_is_packed);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_reduce(const void *in_buf, void *out_buf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

int MPIR_Typerep_pack_stream(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                             MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                             MPI_Aint * actual_pack_bytes, void *stream)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

int MPIR_Typerep_unpack_stream(const void *inbuf, MPI_Aint insize, void *outbuf,
                               MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                               MPI_Aint * actual_unpack_bytes, void *stream)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}
