/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifndef MPIR_STREAM_PREALLOC
#define MPIR_STREAM_PREALLOC 8
#endif

MPIR_Stream MPIR_Stream_direct[MPIR_STREAM_PREALLOC];

MPIR_Object_alloc_t MPIR_Stream_mem = { 0, 0, 0, 0, 0, 0, MPIR_STREAM,
    sizeof(MPIR_Stream), MPIR_Stream_direct,
    MPIR_STREAM_PREALLOC,
    NULL, {0}
};

void MPIR_stream_comm_init(MPIR_Comm * comm)
{
    comm->stream_comm_type = MPIR_STREAM_COMM_NONE;
}

void MPIR_stream_comm_free(MPIR_Comm * comm)
{
    if (comm->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        MPL_free(comm->stream_comm.single.vci_table);
    } else if (comm->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        MPL_free(comm->stream_comm.multiplex.local_streams);
        MPL_free(comm->stream_comm.multiplex.vci_displs);
        MPL_free(comm->stream_comm.multiplex.vci_table);
    }
}

int MPIR_Stream_create_impl(MPIR_Info * info_ptr, MPIR_Stream ** p_stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr;
    stream_ptr = (MPIR_Stream *) MPIR_Handle_obj_alloc(&MPIR_Stream_mem);
    MPIR_ERR_CHKANDJUMP1(!stream_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "MPI_Stream");

    MPIR_Object_set_ref(stream_ptr, 0);
    stream_ptr->vci = 0;

    const char *s_type;
    s_type = MPIR_Info_lookup(info_ptr, "type");

    if (s_type && strcmp(s_type, "cudaStream_t") == 0) {
#ifndef MPL_HAVE_CUDA
        MPIR_Assert(0 && "CUDA not enabled");
#endif
        stream_ptr->type = MPIR_STREAM_GPU;

        /* TODO: proper conversion for each gpu stream type */
        const char *s_value = MPIR_Info_lookup(info_ptr, "value");
        MPIR_Assertp(s_value);
        mpi_errno =
            MPIR_Info_decode_hex(s_value, &stream_ptr->u.gpu_stream, sizeof(MPL_gpu_stream_t));
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        stream_ptr->type = MPIR_STREAM_GENERAL;
    }

    mpi_errno = MPID_Allocate_vci(&stream_ptr->vci);
    MPIR_ERR_CHECK(mpi_errno);

    *p_stream_ptr = stream_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    if (stream_ptr) {
        MPIR_Stream_free_impl(stream_ptr);
    }
    goto fn_exit;
}

int MPIR_Stream_free_impl(MPIR_Stream * stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    int ref_cnt = MPIR_Object_get_ref(stream_ptr);
    MPIR_ERR_CHKANDJUMP(ref_cnt != 0, mpi_errno, MPI_ERR_OTHER, "**cannotfreestream");

    if (stream_ptr->vci) {
        mpi_errno = MPID_Deallocate_vci(stream_ptr->vci);
    }
    MPIR_Handle_obj_free(&MPIR_Stream_mem, stream_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
