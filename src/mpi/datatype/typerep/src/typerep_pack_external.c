/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <dataloop.h>

/* The pack/unpack_external functions do not have the full generality
 * of the regular pack/unpack functions because we only need them for
 * satisfying the MPI-level pack/unpack_external functions.  MPICH
 * internals do not need them. */

int MPIR_Typerep_pack_external(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                               void *outbuf, MPI_Aint * actual_pack_bytes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Segment *segp;

    segp = MPIR_Segment_alloc(inbuf, incount, datatype);
    /* --BEGIN ERROR HANDLING-- */
    if (segp == NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         __func__,
                                         __LINE__,
                                         MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment");
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* NOTE: the use of buffer values and positions in
     * MPI_Pack_external and in MPIR_Segment_pack_external are quite
     * different.  See code or docs or something. */
    *actual_pack_bytes = MPIR_SEGMENT_IGNORE_LAST;
    MPIR_Segment_pack_external32(segp, 0, actual_pack_bytes, outbuf);

    MPIR_Segment_free(segp);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_unpack_external(const void *inbuf, void *outbuf, MPI_Aint outcount,
                                 MPI_Datatype datatype, MPI_Aint * actual_unpack_bytes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Segment *segp;

    segp = MPIR_Segment_alloc(outbuf, outcount, datatype);
    MPIR_ERR_CHKANDJUMP1((segp == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "MPIR_Segment_alloc");

    *actual_unpack_bytes = MPIR_SEGMENT_IGNORE_LAST;
    MPIR_Segment_unpack_external32(segp, 0, actual_unpack_bytes, inbuf);

    MPIR_Segment_free(segp);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
