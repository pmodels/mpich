/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <dataloop.h>
#include "typerep_pre.h"

int MPIR_Typerep_icopy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                       MPIR_Typerep_req * typereq_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_ICOPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_ICOPY);

    MPIR_Memcpy(outbuf, inbuf, num_bytes);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_ICOPY);
    return MPI_SUCCESS;
}

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes)
{
    return MPIR_Typerep_icopy(outbuf, inbuf, num_bytes, NULL);
}

int MPIR_Typerep_ipack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                       MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                       MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typereq_req)
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

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes)
{
    return MPIR_Typerep_ipack(inbuf, incount, datatype, inoffset, outbuf, max_pack_bytes,
                              actual_pack_bytes, NULL);
}


int MPIR_Typerep_iunpack(const void *inbuf, MPI_Aint insize,
                         void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                         MPI_Aint * actual_unpack_bytes, MPIR_Typerep_req * typereq_req)
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

int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize,
                        void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                        MPI_Aint * actual_unpack_bytes)
{
    return MPIR_Typerep_iunpack(inbuf, insize, outbuf, outcount, datatype, outoffset,
                                actual_unpack_bytes, NULL);
}

int MPIR_Typerep_wait(MPIR_Typerep_req typereq_req)
{
    /* All nonblocking operations are actually blocking. Thus, do nothing in wait. */
    return MPI_SUCCESS;
}
